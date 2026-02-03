#include "FrameUploadArena.h"
#include <cstring>
#include <stdexcept>
#include <DirectXTexD3D12.cpp>

using namespace Tsumi::DX12;

void FrameUploadArena::Init(ID3D12Device* device, uint32_t sizeInBytes)
{
	assert(device);
	capacity_ = sizeInBytes;
	currentOffset_ = 0;

	// Upload Heap の作成
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(capacity_);

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer_)
	);

	if (FAILED(hr)) {
		throw std::runtime_error("FrameUploadArena: Failed to create upload buffer.");
	}

	// 常時マップ
	buffer_->Map(0, nullptr, reinterpret_cast<void**>(&cpuPtr_));
	gpuPtr_ = buffer_->GetGPUVirtualAddress();
}

void FrameUploadArena::Finalize()
{
	if (buffer_) {
		buffer_->Unmap(0, nullptr);
		buffer_.Reset();
	}
	cpuPtr_ = nullptr;
	gpuPtr_ = 0;
}

void FrameUploadArena::Begin()
{
	currentOffset_ = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS FrameUploadArena::Upload(const void* data, uint32_t size, uint32_t alignment)
{
	uint32_t alignedSize = AlignUp(size, alignment);

	if (currentOffset_ + alignedSize > capacity_) {
		// エラー処理
		assert(false && "FrameUploadArena overflow!");
		return 0;
	}

	// メモリコピー
	memcpy(cpuPtr_ + currentOffset_, data, size);

	// GPUアドレス計算
	D3D12_GPU_VIRTUAL_ADDRESS va = gpuPtr_ + currentOffset_;
	currentOffset_ += alignedSize;

	return va;
}
