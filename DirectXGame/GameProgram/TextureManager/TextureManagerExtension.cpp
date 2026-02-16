#include "KamataEngine.h"
#include <DirectXTex.h>
#include <list> // 追加
#include <vector>

#pragma comment(lib, "d3d12.lib")

using namespace KamataEngine;
using namespace Microsoft::WRL;

// ★★★ 修正点1：中間リソースを保持しておくためのリストを追加 ★★★
// これがないと、関数を抜けた瞬間にメモリが消えて、GPUが読み込もうとした時にエラーになります
static std::list<ComPtr<ID3D12Resource>> g_KeepAliveResources;

uint32_t TextureManager::LoadDDS(const std::string& fileName) {

	// 1. 空いているテクスチャ番号を探す
	size_t index = useTable_.FindFirst();
	if (index >= kNumDescriptors) {
		return 0;
	}

	// 2. パスの生成
	std::string fullPath = directoryPath_ + fileName;
	wchar_t wfilePath[256];
	MultiByteToWideChar(CP_ACP, 0, fullPath.c_str(), -1, wfilePath, _countof(wfilePath));

	// 3. DDSファイルを読み込む
	DirectX::TexMetadata metadata{};
	DirectX::ScratchImage scratchImg{};
	HRESULT result = LoadFromDDSFile(wfilePath, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, &metadata, scratchImg);

	if (FAILED(result)) {
		return 0;
	}

	// 4. リソース（GPUメモリ）の生成
	D3D12_HEAP_PROPERTIES uploadHeapProp{};
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC texResDesc{};
	texResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texResDesc.Format = metadata.format;
	texResDesc.Width = metadata.width;
	texResDesc.Height = (UINT)metadata.height;
	texResDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	texResDesc.MipLevels = (UINT16)metadata.mipLevels;
	texResDesc.SampleDesc.Count = 1;

	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	ComPtr<ID3D12Resource> textureBuffer;

	result = device_->CreateCommittedResource(&texHeapProp, D3D12_HEAP_FLAG_NONE, &texResDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureBuffer));
	assert(SUCCEEDED(result));

	// 5. テクスチャデータの転送準備
	const DirectX::Image* images = scratchImg.GetImages();
	size_t nImages = scratchImg.GetImageCount();

	std::vector<D3D12_SUBRESOURCE_DATA> subresources(nImages);
	for (size_t i = 0; i < nImages; ++i) {
		subresources[i].pData = images[i].pixels;
		subresources[i].RowPitch = images[i].rowPitch;
		subresources[i].SlicePitch = images[i].slicePitch;
	}

	uint64_t intermediateSize = GetRequiredIntermediateSize(textureBuffer.Get(), 0, (UINT)subresources.size());

	ComPtr<ID3D12Resource> intermediateResource;
	D3D12_RESOURCE_DESC intermediateDesc = CD3DX12_RESOURCE_DESC::Buffer(intermediateSize);

	device_->CreateCommittedResource(&uploadHeapProp, D3D12_HEAP_FLAG_NONE, &intermediateDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource));

	// コマンドリストを使って転送命令を記録
	// (この時点では命令するだけで、実際の実行はPostDrawで行われます)
	auto dxCommon = DirectXCommon::GetInstance();
	auto commandList = dxCommon->GetCommandList();

	UpdateSubresources(commandList, textureBuffer.Get(), intermediateResource.Get(), 0, 0, (UINT)subresources.size(), subresources.data());

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &barrier);

	// ★★★ 修正点2：中間リソースをリストに入れて寿命を延ばす ★★★
	// これで、GPUが処理を終えるまでメモリが解放されなくなります
	g_KeepAliveResources.push_back(intermediateResource);

	// 6. シェーダーリソースビュー(SRV)の作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = (UINT)metadata.mipLevels;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap_->GetCPUDescriptorHandleForHeapStart(), (INT)index, device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(descriptorHeap_->GetGPUDescriptorHandleForHeapStart(), (INT)index, device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	device_->CreateShaderResourceView(textureBuffer.Get(), &srvDesc, cpuHandle);

	// 7. 管理配列に登録
	textures_[index].resource = textureBuffer;
	textures_[index].cpuDescHandleSRV = cpuHandle;
	textures_[index].gpuDescHandleSRV = gpuHandle;
	textures_[index].name = fileName;

	useTable_.Set(index);

	return (uint32_t)index;
}