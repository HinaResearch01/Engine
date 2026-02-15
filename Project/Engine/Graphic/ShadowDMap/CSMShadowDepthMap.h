#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>
#include "DX12/Desc/DescriptorUtils.h"

namespace Tsumi::DX12 { 
class DX12Manager;
class CommandContext;
}

namespace Tsumi::Graphic {

class CSMShadowDepthMap {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CSMShadowDepthMap() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~CSMShadowDepthMap() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(uint32_t size, uint32_t cascadeCount);

	/// <summary>
	/// リサイズ
	/// </summary>
	void Resize(uint32_t size, uint32_t cascadeCount);

#pragma region Accessor
	ID3D12Resource* GetResource() const { return depth_.Get(); }
	uint32_t GetSize() const { return size_; }
	uint32_t GetCascadeCount() const { return cascadeCount_; }

	// SRV（Texture2DArray）
	const DX12::DescriptorHandle& GetSRV() const { return srv_; }

	// DSV（cascadeごと）
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(uint32_t cascadeIdx) const;
	const D3D12_CPU_DESCRIPTOR_HANDLE* GetDSVPtr(uint32_t cascadeIdx) const;

	// Resource Barrier
	void TransitionToWrite(DX12::CommandContext& cmd);
	void TransitionToRead(DX12::CommandContext& cmd);
#pragma endregion

	// フォーマット規約
	static constexpr DXGI_FORMAT kDSVFormat = DXGI_FORMAT_D32_FLOAT;
	static constexpr DXGI_FORMAT kSRVFormat = DXGI_FORMAT_R32_FLOAT;

private:
	void Create(uint32_t size, uint32_t cascadeCount);
	void CreateViews();

	static DXGI_FORMAT MakeTypelessDepth_(DXGI_FORMAT dsvFmt);

private:
	uint32_t size_ = 0;
	uint32_t cascadeCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> depth_;

	// DSV heap（cascadeCount個）
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	uint32_t dsvIncSize_ = 0;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvCpu_; // [cascadeCount]

	// SRV（persistent）
	DX12::DescriptorHandle srv_{};

	D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
};

}
