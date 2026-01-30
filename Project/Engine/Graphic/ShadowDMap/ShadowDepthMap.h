#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DX12/Desc/DescriptorUtils.h"

// 前方宣言
namespace Tsumi::DX12{ class DX12Manager; }

namespace Tsumi::Graphic {

/* ShadowDepthMap（Resource 所有クラス） */
class ShadowDepthMap {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	ShadowDepthMap() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~ShadowDepthMap() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(uint32_t size);
	
	/// <summary>
	/// 再生成
	/// </summary>
	void Resize(uint32_t size);

#pragma region Accessor 
	ID3D12Resource* GetResource() const { return depth_.Get(); }
	uint32_t GetSize() const { return size_; }
	const DX12::DescriptorHandle& GetSRV() const { return srv_; }
	D3D12_CPU_DESCRIPTOR_HANDLE   GetDSV() const { return dsvCpu_; }
#pragma endregion

	// DSV / SRV 用フォーマット規約
	static constexpr DXGI_FORMAT kDSVFormat = DXGI_FORMAT_D32_FLOAT;
	static constexpr DXGI_FORMAT kSRVFormat = DXGI_FORMAT_R32_FLOAT;

private:
	/// <summary>
	/// 生成
	/// </summary>
	void Create(uint32_t size);
	void CreateViews();

private:
	uint32_t size_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> depth_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCpu_{ 0 };
	DX12::DescriptorHandle srv_{};
};

}
