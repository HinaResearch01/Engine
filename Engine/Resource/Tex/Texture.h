#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "DX12/Desc/DescriptorAllocator.h"

namespace Tsumi::DX12 {
class DX12Manager;
class DescriptorAllocator;
class CommandContext;
}

namespace Tsumi::Resource {

struct TextureDesc {
	UINT width = 0;
	UINT height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT mipLevels = 1;
};

class Texture {

public:
	/// <summary>
	/// コンストラクタ	
	/// </summary>
	Texture() = default;
	
	/// <summary>
	/// デストラクタ
	/// </summary>
	~Texture() = default;

#pragma region Accessor
	ID3D12Resource* GetResource() const { return resource_.Get(); }
	const TextureDesc& GetDesc() const { return desc_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvDesc_.gpuHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCpuHandle() const { return srvDesc_.cpuHandle; }
#pragma endregion

private:
	friend class TextureManager;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	TextureDesc desc_;
	DX12::DescAlloc srvDesc_;
};

}