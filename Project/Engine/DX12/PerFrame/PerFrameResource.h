#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DX12/Desc/DescriptorAllocator.h"

// 前方宣言
namespace Tsumi::DX12{
class DX12Manager;
class DescriptorAllocator;
}

namespace Tsumi::DX12 {

/* フレームごとのリソース管理クラス */
class PerFrameResource {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	PerFrameResource() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~PerFrameResource();

	/// <summary>
	/// 初期化処理
	/// </summary>
	HRESULT Init(ID3D12Device* device, size_t uploadBufferSize);

	/// <summary>
	/// フレーム開始処理
	/// </summary>
	void BeginFrame(uint32_t frameIndex);

	/// <summary>
	/// フレーム切り替え時に呼ぶ
	/// </summary>
	bool ResetForRecord();

	/// <summary>
	/// フレーム内の線形アロケーション
	/// </summary>
	bool Allocate(size_t bytes, size_t alignment, size_t& outOffset);

#pragma region Accessor
	ID3D12Resource* GetUploadBuffer() const { return uploadBuffer_.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetUploadBufferGPUAddr() const {
		return uploadBuffer_ ? uploadBuffer_->GetGPUVirtualAddress() : 0;
	}
	uint8_t* GetMappedPtr() const { return mappedUploadPtr_; }
	size_t GetUploadBufferSize() const { return uploadBufferSize_; }
	void SetFenceValue(uint64_t v) { fenceValue_ = v; }
	uint64_t GetFenceValue() const { return fenceValue_; }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
	uint8_t* mappedUploadPtr_ = nullptr;
	size_t uploadBufferSize_ = 0;
	uint64_t fenceValue_ = 0;

	size_t   currentOffset_ = 0;
};
}