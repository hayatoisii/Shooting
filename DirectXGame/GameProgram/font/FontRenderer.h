#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "KamataEngine.h"

// TTF フォントから起動時に文字アトラス(PNG)を生成し、スプライトで日本語テキストを描画するクラス。
// エンジンがメモリからテクスチャを作れないため、一度 PNG に書き出して TextureManager で読み込む方式。
class FontRenderer {
public:
	FontRenderer() = default;
	~FontRenderer();

	// ttfPath      : 読み込む .ttf の相対/絶対パス（ワイド文字）
	// fontFamily   : フォントのファミリ名（例: L"Noto Sans JP"）
	// pixelHeight  : アトラスに焼き込む文字の高さ(px)。大きいほど綺麗だがアトラスも大きくなる
	// atlasOutPath : 生成する PNG の出力先（例: "Resources/generatedfont/notosans.png"）
	// atlasLoadName: TextureManager::Load に渡す名前（例: "generatedfont/notosans.png"）
	// extraChars   : かな/英数字/記号に加えて必ず焼き込みたい文字（漢字など）をまとめたワイド文字列
	bool Initialize(const std::wstring& ttfPath, const std::wstring& fontFamily, int pixelHeight,
	    const std::string& atlasOutPath, const std::string& atlasLoadName, const std::wstring& extraChars);

	bool IsReady() const { return ready_; }

	// 1フレームの描画開始時に呼ぶ（内部スプライトプールの使用位置をリセット）
	void NewFrame();

	// UTF-8 文字列を (x, y) を左上として描画する。drawHeight は表示上の文字の高さ(px)。
	// 戻り値は描画した文字列の横幅(px)。
	float DrawString(const std::string& utf8Text, float x, float y, float drawHeight,
	    const KamataEngine::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f});

	// 描画したときの横幅(px)を計算する（描画はしない）。
	float MeasureWidth(const std::string& utf8Text, float drawHeight) const;

private:
	struct Glyph {
		float u = 0.0f; // アトラス上の左上 X (texel)
		float v = 0.0f; // アトラス上の左上 Y (texel)
		float w = 0.0f; // 幅 (texel)
		float h = 0.0f; // 高さ (texel)
		float advance = 0.0f; // 次の文字までの送り幅 (texel)
	};

	KamataEngine::Sprite* AcquireSprite();
	static std::vector<char32_t> DecodeUtf8(const std::string& utf8);

	std::unordered_map<char32_t, Glyph> glyphs_;
	int pixelHeight_ = 48;
	int atlasWidth_ = 0;
	int atlasHeight_ = 0;
	uint32_t atlasTexture_ = 0;
	bool ready_ = false;

	std::vector<KamataEngine::Sprite*> spritePool_;
	size_t spriteCursor_ = 0;
};
