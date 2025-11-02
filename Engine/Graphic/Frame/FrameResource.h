#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DX12/Desc/DescriptorAllocator.h"

namespace Tsumi::Graphic {

class FrameResource {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameResource() = default;
	
	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameResource() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	bool Init();

	/// <summary>
	/// 
	/// </summary>
	void ResetForRecord();

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
};
}