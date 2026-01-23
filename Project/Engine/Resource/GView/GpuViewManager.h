#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <cassert>
#include "DX12/Desc/DescriptorAllocator.h"

// 前方宣言
namespace Tsumi::DX12 { class DX12Manager; }

namespace Tsumi::Resource {

/* SRV/UAV の生成保持管理するクラス */
class GpuViewManager {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	GpuViewManager() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~GpuViewManager() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	// --------------------------------------------------
	// SRV
	// --------------------------------------------------
	// Texture SRV
	void RegisterTextureSRV(
		const std::string& name,
		ID3D12Resource* resource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
	// StructuredBuffer SRV
	void RegisterStructuredBufferSRV(
		const std::string& name,
		ID3D12Resource* resource,
		UINT stride,
		UINT elementCount);

	// --------------------------------------------------
	// UAV
	// --------------------------------------------------
	void RegiszterUAV(
		const std::string& name,
		ID3D12Resource* resource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);

	/// <summary>
	/// 全破棄
	/// </summary>
	void Clear();

#pragma region Accessor
	bool HasSRV(const std::string& name) const {
		return srvs_.contains(name);
	}
	bool HasUAV(const std::string& name) const {
		return uavs_.contains(name);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetSRV(const std::string& name) const {
		auto it = srvs_.find(name);
		assert(it != srvs_.end());
		return it->second.desc.gpuHandle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetUAV(const std::string& name) const {
		auto it = uavs_.find(name);
		assert(it != uavs_.end());
		return it->second.desc.gpuHandle;
	}
#pragma endregion

private:
	struct ViewEntry {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		DX12::DescAlloc desc;
	};

private:
	std::unordered_map<std::string, ViewEntry> srvs_;
	std::unordered_map<std::string, ViewEntry> uavs_;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}