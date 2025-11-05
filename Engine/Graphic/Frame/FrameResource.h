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

namespace Tsumi::Graphic {

/* フレームごとのリソース管理クラス */
class FrameResource {

private: // シングルトン
	FrameResource();
	~FrameResource() = default;
	FrameResource(const FrameResource&) = delete;
	FrameResource& operator=(const FrameResource&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static FrameResource* GetInstance() {
		static FrameResource instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	bool Init();

	/// <summary>
	/// フレーム切り替え時に呼ぶ
	/// </summary>
	bool ResetForRecord();

#pragma region Accessor
	ID3D12CommandAllocator* GetCommandAllocator() const { return cmdAllocator_.Get(); }
	DX12::DescriptorAllocator* GetDescriptorAllocator() { return descAllocaor_.get(); }
	ID3D12Resource* GetUploadBuffer() const { return uploadBuffer_.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetUploadBufferGPUAddr() const {
		return uploadBuffer_ ? uploadBuffer_->GetGPUVirtualAddress() : 0;
	}
	void SetFenceValue(uint64_t v) { fenceValue_ = v; }
	uint64_t GetFenceValue() const { return fenceValue_; }
	size_t GetUploadBufferSize() const { return uploadBufferSize_; }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator_;
	std::unique_ptr<DX12::DescriptorAllocator> descAllocaor_;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
	uint8_t* mappedUploadPtr_ = nullptr;
	size_t uploadBufferSize_ = 0;
	uint64_t fenceValue_ = 0;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
	DX12::DescriptorAllocator* descAlloc_ = nullptr;
};
}