#include "TextureConverter.h"
#include <Windows.h>

using namespace DirectX;

std::wstring TextureConverter::ConvertMultiByteStringToWideString(const std::string& mString) {
	int filePathBufferSize = MultiByteToWideChar(CP_ACP, 0, mString.c_str(), -1, nullptr, 0);

	std::wstring wString;
	wString.resize(filePathBufferSize);

	MultiByteToWideChar(CP_ACP, 0, mString.c_str(), -1, &wString[0], filePathBufferSize);

	return wString;
}

void TextureConverter::SeparateFilePath(const std::wstring& filePath) {
	size_t pos1;
	std::wstring exceptExt;

	// 区切り文字'.'が出てくる一番最後の部分を検索
	pos1 = filePath.rfind('.');
	// 検索がヒットしたら
	if (pos1 != std::wstring::npos) {
		// 区切り文字の後ろをファイル拡張子として保存
		fileExt_ = filePath.substr(pos1 + 1, filePath.size() - pos1 - 1);
		// 区切り文字の前までを抜き出す
		exceptExt = filePath.substr(0, pos1);
	} else {
		fileExt_ = L"";
		exceptExt = filePath;
	}
	pos1 = exceptExt.rfind('\\');
	if (pos1 != std::wstring::npos) {
		// 区切り文字前までをディレクトリパスとして保存
		directoryPath_ = exceptExt.substr(pos1 + 1, exceptExt.size() - pos1 - 1);
		return;
	}
	// 区切り文字'/'が出てくる一番最後の部分を検索
	// 区切り文字'\\'が出てくる一番最後の部分を検索
	pos1 = exceptExt.rfind('\\');
	if (pos1 != std::wstring::npos) {
		// 区切り文字の前までをディレクトリパスとして保存
		directoryPath_ = exceptExt.substr(0, pos1 + 1);
		// 区切り文字の後ろをファイル名として保存
		fileName_ = exceptExt.substr(pos1 + 1, exceptExt.size() - pos1 - 1);
		return;
	}
	directoryPath_ = L"";
	fileName_ = exceptExt;
}

// この関数をまるごと入れ替えてください
void TextureConverter::SaveDDSTextureToFile() {
	HRESULT result;

	// =====================================================================
	// 1. MipMap生成 (資料の黄色い部分 その1)
	// =====================================================================
	DirectX::ScratchImage mipChain;
	result = GenerateMipMaps(
	    scratchImage_.GetImages(),     // 元の画像データ
	    scratchImage_.GetImageCount(), // 画像の枚数
	    scratchImage_.GetMetadata(),   // 画像の情報
	    DirectX::TEX_FILTER_DEFAULT,   // フィルタ設定
	    0,                             // ミップレベル (0指定で全レベル作成)
	    mipChain                       // 結果の出力先
	);

	if (SUCCEEDED(result)) {
		// 生成に成功したら、元の画像をミップマップ付きのものに置き換える
		scratchImage_ = std::move(mipChain);
		metadata_ = scratchImage_.GetMetadata();
	}

	// =====================================================================
	// 2. 圧縮処理 (資料の黄色い部分 その2)
	// =====================================================================
	// 圧縮処理はMipMap生成の「後」に行うのが鉄則です
	DirectX::ScratchImage converted;
	result = Compress(
	    scratchImage_.GetImages(),     // 元画像 (MipMap付き)
	    scratchImage_.GetImageCount(), // 画像数
	    metadata_,                     // 画像の情報
	    DXGI_FORMAT_BC7_UNORM_SRGB,    // ★BC7形式 (高圧縮・高画質)
	    DirectX::TEX_COMPRESS_BC7_QUICK | DirectX::TEX_COMPRESS_SRGB_OUT | DirectX::TEX_COMPRESS_PARALLEL,
	    1.0f,     // アルファ閾値
	    converted // 結果の出力先
	);

	if (SUCCEEDED(result)) {
		// 圧縮に成功したら、元の画像を圧縮済みのものに置き換える
		scratchImage_ = std::move(converted);
		metadata_ = scratchImage_.GetMetadata();
	}

	// =====================================================================
	// 3. ファイルへの書き出し
	// =====================================================================
	// 読み込んだテクスチャをSRGBとして扱う
	metadata_.format = DirectX::MakeSRGB(metadata_.format);

	// 出力ファイル名を設定する (ディレクトリ + ファイル名 + .dds)
	std::wstring filePath = directoryPath_ + fileName_ + L".dds";

	// DDSファイル書き出し
	result = SaveToDDSFile(scratchImage_.GetImages(), scratchImage_.GetImageCount(), metadata_, DirectX::DDS_FLAGS_NONE, filePath.c_str());

	assert(SUCCEEDED(result));
}

void TextureConverter::LoadWICTextureFromFile(const std::string& filePath) {
	std::wstring wFilePath = ConvertMultiByteStringToWideString(filePath);

	HRESULT result = LoadFromWICFile(wFilePath.c_str(), WIC_FLAGS_NONE, &metadata_, scratchImage_);
	assert(SUCCEEDED(result));
	(void)result;

	// ファイルパスとファイル名を分離する
	SeparateFilePath(wFilePath);
}

void TextureConverter::ConvertTextureWICToDDS(const std::string& filePath) {
	// 1. 画像読み込み
	LoadWICTextureFromFile(filePath);

	// 2. 圧縮処理 (ここはそのままでOK)
	DirectX::ScratchImage compressed;
	HRESULT result = DirectX::Compress(
	    scratchImage_.GetImages(), scratchImage_.GetImageCount(), metadata_, DXGI_FORMAT_BC7_UNORM_SRGB,
	    DirectX::TEX_COMPRESS_BC7_QUICK | DirectX::TEX_COMPRESS_SRGB_OUT | DirectX::TEX_COMPRESS_PARALLEL, 1.0f, compressed);

	if (SUCCEEDED(result)) {
		scratchImage_ = std::move(compressed);
		metadata_ = scratchImage_.GetMetadata();
	}

	// 元のファイルパス（例: Resources/mario.png）をワイド文字にする
	std::wstring wFilePath = ConvertMultiByteStringToWideString(filePath);

	// これなら "Resources/" というフォルダ情報もそのまま残る！
	std::wstring ddsPath = wFilePath;
	size_t pos = ddsPath.find(L".png");
	if (pos != std::wstring::npos) {
		ddsPath.replace(pos, 4, L".dds");
	}

	result = SaveToDDSFile(scratchImage_.GetImages(), scratchImage_.GetImageCount(), metadata_, DirectX::DDS_FLAGS_NONE, ddsPath.c_str());
	assert(SUCCEEDED(result));
}