#pragma once
#include <DirectXTex.h>
#include <Windows.h>
#include <string>

class TextureConverter {
public:
	void ConvertTextureWICToDDS(const std::string& filePath);
	void LoadWICTextureFromFile(const std::string& filePath);

private:
	static std::wstring ConvertMultiByteStringToWideString(const std::string& mString);

	void SeparateFilePath(const std::wstring& filePath);

	void SaveDDSTextureToFile();

	// 画像の情報
	DirectX::TexMetadata metadata_;
	// 画像イメージのコンテナ
	DirectX::ScratchImage scratchImage_;

	// ディレクトリパス
	std::wstring directoryPath_;
	// ファイル名
	std::wstring fileName_;
	// ファイル拡張子
	std::wstring fileExt_;
};