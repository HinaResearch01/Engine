#include "FrameUploadArena.h"
#include <cstring>

using namespace Tsumi::DX12;

void FrameUploadArena::Init(ID3D12Device* device, uint32_t sizeInBytes)
{
	assert(device);
	capacity_ = sizeInBytes;
	offset_ = 0;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = sizeInBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer_));
	assert(SUCCEEDED(hr));

	hr = buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped_));
	assert(SUCCEEDED(hr));
}

void FrameUploadArena::Finalize()
{
	if (buffer_) {
		buffer_->Unmap(0, nullptr);
		buffer_.Reset();
	}
	mapped_ = nullptr;
	capacity_ = 0;
	offset_ = 0;
}

void FrameUploadArena::BeginFrame()
{
	offset_ = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS FrameUploadArena::Upload(const void* data, uint32_t size, uint32_t alignment)
{
	assert(buffer_);
	assert(mapped_);
	assert(alignment != 0);

	const uint32_t aligned = AlignUp(size, alignment);
	assert(offset_ + aligned <= capacity_);

	std::memcpy(mapped_ + offset_, data, size);

	D3D12_GPU_VIRTUAL_ADDRESS va = buffer_->GetGPUVirtualAddress() + offset_;
	offset_ += aligned;
	return va;
}
