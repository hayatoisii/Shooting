#include "FontRenderer.h"

#include <algorithm>
#include <filesystem>
#include <set>

#include <Windows.h>
#include <wincodec.h>

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Windowscodecs.lib")

using namespace KamataEngine;

namespace {

constexpr int kAtlasWidth = 1024; // アトラス横幅(px)。足りなければ縦に伸ばす
constexpr int kGlyphPadding = 2;  // 文字同士の隙間(px)

// BGRA トップダウンの生ピクセルを PNG として保存する
bool SavePngBGRA(const std::wstring& path, UINT width, UINT height, const std::vector<uint8_t>& pixels) {
	// COM は WinApp 側で初期化済みだが、念のため呼ぶ（既に初期化済みなら無害）
	const HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	bool ok = false;
	IWICImagingFactory* factory = nullptr;
	IWICStream* stream = nullptr;
	IWICBitmapEncoder* encoder = nullptr;
	IWICBitmapFrameEncode* frame = nullptr;
	IPropertyBag2* props = nullptr;

	if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
		if (SUCCEEDED(factory->CreateStream(&stream)) &&
		    SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
		    SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
		    SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
		    SUCCEEDED(encoder->CreateNewFrame(&frame, &props)) &&
		    SUCCEEDED(frame->Initialize(props))) {
			WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
			const UINT stride = width * 4;
			const UINT bufferSize = stride * height;
			if (SUCCEEDED(frame->SetSize(width, height)) &&
			    SUCCEEDED(frame->SetPixelFormat(&format)) &&
			    SUCCEEDED(frame->WritePixels(height, stride, bufferSize, const_cast<BYTE*>(pixels.data()))) &&
			    SUCCEEDED(frame->Commit()) &&
			    SUCCEEDED(encoder->Commit())) {
				ok = true;
			}
		}
	}

	if (props) props->Release();
	if (frame) frame->Release();
	if (encoder) encoder->Release();
	if (stream) stream->Release();
	if (factory) factory->Release();

	if (SUCCEEDED(coInit)) {
		CoUninitialize();
	}
	return ok;
}

} // namespace

FontRenderer::~FontRenderer() {
	for (Sprite* sprite : spritePool_) {
		delete sprite;
	}
	spritePool_.clear();
}

std::vector<char32_t> FontRenderer::DecodeUtf8(const std::string& utf8) {
	std::vector<char32_t> out;
	out.reserve(utf8.size());
	size_t i = 0;
	const size_t n = utf8.size();
	while (i < n) {
		const unsigned char c = static_cast<unsigned char>(utf8[i]);
		char32_t cp = 0;
		int extra = 0;
		if (c < 0x80) {
			cp = static_cast<char32_t>(c);
			extra = 0;
		} else if ((c >> 5) == 0x6) {
			cp = static_cast<char32_t>(c & 0x1F);
			extra = 1;
		} else if ((c >> 4) == 0xE) {
			cp = static_cast<char32_t>(c & 0x0F);
			extra = 2;
		} else if ((c >> 3) == 0x1E) {
			cp = static_cast<char32_t>(c & 0x07);
			extra = 3;
		} else {
			++i;
			continue;
		}
		if (i + static_cast<size_t>(extra) >= n) {
			break;
		}
		bool valid = true;
		for (int k = 1; k <= extra; ++k) {
			const unsigned char cc = static_cast<unsigned char>(utf8[i + static_cast<size_t>(k)]);
			if ((cc >> 6) != 0x2) {
				valid = false;
				break;
			}
			cp = static_cast<char32_t>((cp << 6) | static_cast<char32_t>(cc & 0x3F));
		}
		if (valid) {
			out.push_back(cp);
		}
		i += static_cast<size_t>(extra) + 1;
	}
	return out;
}

bool FontRenderer::Initialize(const std::wstring& ttfPath, const std::wstring& fontFamily, int pixelHeight,
    const std::string& atlasOutPath, const std::string& atlasLoadName, const std::wstring& extraChars) {
	pixelHeight_ = (pixelHeight > 0) ? pixelHeight : 48;

	// --- 焼き込む文字集合を作る（重複除去のため set を使用） ---
	std::set<wchar_t> charSet;
	for (wchar_t c = 0x20; c <= 0x7E; ++c) {
		charSet.insert(c); // ASCII
	}
	for (wchar_t c = 0x3041; c <= 0x3096; ++c) {
		charSet.insert(c); // ひらがな
	}
	for (wchar_t c = 0x30A1; c <= 0x30FA; ++c) {
		charSet.insert(c); // カタカナ
	}
	// よく使う記号・約物
	const wchar_t kSymbols[] = {0x3000, 0x3001, 0x3002, 0x300C, 0x300D, 0x300E, 0x300F, 0x3005, 0x2026, 0x30FB,
	    0x30FC, 0x309B, 0x309C, 0xFF01, 0xFF1F, 0xFF08, 0xFF09, 0xFF1A, 0xFF1B, 0x30FD, 0x30FE, 0x309D, 0x309E};
	for (wchar_t c : kSymbols) {
		charSet.insert(c);
	}
	for (wchar_t c : extraChars) {
		if (c != 0) {
			charSet.insert(c);
		}
	}

	std::vector<wchar_t> chars(charSet.begin(), charSet.end());
	if (chars.empty()) {
		return false;
	}

	// --- GDI でフォントを用意 ---
	// 実行ディレクトリの違いに対応するため、複数パスを試す
	std::wstring usedFontPath = ttfPath;
	int fontAdded = AddFontResourceExW(ttfPath.c_str(), FR_PRIVATE, nullptr);
	if (fontAdded == 0) {
		const std::wstring altPath = L"DirectXGame/" + ttfPath;
		fontAdded = AddFontResourceExW(altPath.c_str(), FR_PRIVATE, nullptr);
		if (fontAdded > 0) {
			usedFontPath = altPath;
		}
	}

	HDC screenDC = GetDC(nullptr);
	HDC dc = CreateCompatibleDC(screenDC);
	if (!dc) {
		if (screenDC) {
			ReleaseDC(nullptr, screenDC);
		}
		if (fontAdded > 0) {
			RemoveFontResourceExW(usedFontPath.c_str(), FR_PRIVATE, nullptr);
		}
		return false;
	}

	LOGFONTW lf{};
	lf.lfHeight = -pixelHeight_;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfQuality = ANTIALIASED_QUALITY; // ClearType を避けてグレースケールアンチエイリアスにする
	wcsncpy_s(lf.lfFaceName, fontFamily.c_str(), _TRUNCATE);
	HFONT font = CreateFontIndirectW(&lf);
	HFONT oldFont = static_cast<HFONT>(SelectObject(dc, font));

	TEXTMETRICW tm{};
	GetTextMetricsW(dc, &tm);
	const int cellH = static_cast<int>(tm.tmHeight) + kGlyphPadding;

	// --- レイアウト（棚詰め） ---
	struct Placed {
		wchar_t ch;
		int x, y, w;
	};
	std::vector<Placed> placed;
	placed.reserve(chars.size());

	int penX = kGlyphPadding;
	int penY = kGlyphPadding;
	int maxRowBottom = penY + cellH;
	for (wchar_t ch : chars) {
		SIZE sz{};
		GetTextExtentPoint32W(dc, &ch, 1, &sz);
		int w = static_cast<int>(sz.cx);
		if (w <= 0) {
			w = pixelHeight_ / 2;
		}
		if (penX + w + kGlyphPadding > kAtlasWidth) {
			penX = kGlyphPadding;
			penY += cellH + kGlyphPadding;
		}
		placed.push_back({ch, penX, penY, w});
		penX += w + kGlyphPadding;
		maxRowBottom = penY + cellH + kGlyphPadding;
	}

	atlasWidth_ = kAtlasWidth;
	atlasHeight_ = maxRowBottom;

	// --- 32bit トップダウン DIB を作り、文字を白で描画 ---
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = atlasWidth_;
	bmi.bmiHeader.biHeight = -atlasHeight_; // 負でトップダウン
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* dibBits = nullptr;
	HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &dibBits, nullptr, 0);
	HBITMAP oldBmp = nullptr;
	bool generated = false;

	if (dib && dibBits) {
		oldBmp = static_cast<HBITMAP>(SelectObject(dc, dib));
		const size_t byteCount = static_cast<size_t>(atlasWidth_) * static_cast<size_t>(atlasHeight_) * 4u;
		memset(dibBits, 0, byteCount);

		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, RGB(255, 255, 255));

		for (const Placed& p : placed) {
			TextOutW(dc, p.x, p.y, &p.ch, 1);
		}
		GdiFlush();

		// GDI は 32bit DIB に描画してもアルファを書かないので、輝度からアルファを合成する
		std::vector<uint8_t> pixels(byteCount);
		memcpy(pixels.data(), dibBits, byteCount);
		for (size_t i = 0; i + 3 < byteCount; i += 4) {
			const uint8_t b = pixels[i + 0];
			const uint8_t g = pixels[i + 1];
			const uint8_t r = pixels[i + 2];
			const uint8_t a = (std::max)(r, (std::max)(g, b));
			pixels[i + 0] = 255; // B
			pixels[i + 1] = 255; // G
			pixels[i + 2] = 255; // R
			pixels[i + 3] = a;   // A
		}

		// 出力先ディレクトリを作成
		std::error_code ec;
		std::filesystem::create_directories(std::filesystem::path(atlasOutPath).parent_path(), ec);

		const std::wstring wpath = std::filesystem::path(atlasOutPath).wstring();
		generated = SavePngBGRA(wpath, static_cast<UINT>(atlasWidth_), static_cast<UINT>(atlasHeight_), pixels);

		if (generated) {
			// グリフ情報を保存（texel 単位）
			for (const Placed& p : placed) {
				Glyph glyph;
				glyph.u = static_cast<float>(p.x);
				glyph.v = static_cast<float>(p.y);
				glyph.w = static_cast<float>(p.w);
				glyph.h = static_cast<float>(cellH - kGlyphPadding);
				glyph.advance = static_cast<float>(p.w);
				glyphs_[static_cast<char32_t>(p.ch)] = glyph;
			}
		}
	}

	// --- GDI 後始末 ---
	if (oldBmp) {
		SelectObject(dc, oldBmp);
	}
	if (dib) {
		DeleteObject(dib);
	}
	SelectObject(dc, oldFont);
	if (font) {
		DeleteObject(font);
	}
	DeleteDC(dc);
	if (screenDC) {
		ReleaseDC(nullptr, screenDC);
	}
	if (fontAdded > 0) {
		RemoveFontResourceExW(usedFontPath.c_str(), FR_PRIVATE, nullptr);
	}

	if (!generated) {
		return false;
	}

	// --- 生成した PNG をエンジンに読み込ませる ---
	atlasTexture_ = TextureManager::Load(atlasLoadName);
	if (atlasTexture_ == 0) {
		return false;
	}

	ready_ = true;
	return true;
}

void FontRenderer::NewFrame() { spriteCursor_ = 0; }

KamataEngine::Sprite* FontRenderer::AcquireSprite() {
	if (spriteCursor_ < spritePool_.size()) {
		return spritePool_[spriteCursor_++];
	}
	Sprite* sprite = Sprite::Create(atlasTexture_, {0.0f, 0.0f});
	if (sprite) {
		sprite->SetAnchorPoint({0.0f, 0.0f});
	}
	spritePool_.push_back(sprite);
	++spriteCursor_;
	return sprite;
}

float FontRenderer::DrawString(const std::string& utf8Text, float x, float y, float drawHeight,
    const KamataEngine::Vector4& color) {
	if (!ready_) {
		return 0.0f;
	}
	const float scale = (pixelHeight_ > 0) ? (drawHeight / static_cast<float>(pixelHeight_)) : 1.0f;
	float penX = x;

	const std::vector<char32_t> codepoints = DecodeUtf8(utf8Text);
	for (char32_t cp : codepoints) {
		auto it = glyphs_.find(cp);
		if (it == glyphs_.end()) {
			penX += drawHeight * 0.4f; // 未収録文字は空白として送る
			continue;
		}
		const Glyph& g = it->second;
		Sprite* sprite = AcquireSprite();
		if (sprite) {
			sprite->SetTextureRect({g.u, g.v}, {g.w, g.h});
			sprite->SetSize({g.w * scale, g.h * scale});
			sprite->SetPosition({penX, y});
			sprite->SetColor(color);
			sprite->Draw();
		}
		penX += g.advance * scale;
	}
	return penX - x;
}

float FontRenderer::MeasureWidth(const std::string& utf8Text, float drawHeight) const {
	if (!ready_) {
		return 0.0f;
	}
	const float scale = (pixelHeight_ > 0) ? (drawHeight / static_cast<float>(pixelHeight_)) : 1.0f;
	float width = 0.0f;
	const std::vector<char32_t> codepoints = DecodeUtf8(utf8Text);
	for (char32_t cp : codepoints) {
		auto it = glyphs_.find(cp);
		if (it == glyphs_.end()) {
			width += drawHeight * 0.4f;
			continue;
		}
		width += it->second.advance * scale;
	}
	return width;
}
