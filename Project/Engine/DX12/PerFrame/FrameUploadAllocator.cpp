#include "FrameUploadAllocator.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;

void FrameUploadAllocator::Init(ID3D12Device* device, uint32_t bytes)
{
	assert(device);
	size_ = bytes;

	auto desc = CD3DX12_RESOURCE_DESC::Buffer(bytes);
	HRESULT hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&upload_));
	assert(SUCCEEDED(hr));

	CD3DX12_RANGE range(0, 0);
	hr = upload_->Map(0, &range, reinterpret_cast<void**>(&mapped_));
	assert(SUCCEEDED(hr));

	baseGpu_ = upload_->GetGPUVirtualAddress();
	offset_ = 0;
}

void FrameUploadAllocator::Reset()
{
	offset_ = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS FrameUploadAllocator::Upload(const void* data, uint32_t bytes, uint32_t align)
{
	assert(mapped_);
	assert(data);
	assert(bytes > 0);

	const uint32_t need = AlignUp(bytes, align);
	assert(offset_ + need <= size_);

	std::memcpy(mapped_ + offset_, data, bytes);
	D3D12_GPU_VIRTUAL_ADDRESS gpu = baseGpu_ + offset_;
	offset_ += need;
	return gpu;
}
