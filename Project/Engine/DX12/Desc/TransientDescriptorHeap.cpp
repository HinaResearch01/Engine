#include "TransientDescriptorHeap.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;

TransientDescriptorHeap::TransientDescriptorHeap(DX12Manager* ptr)
	: dx12Mgr_(ptr)
{}

void TransientDescriptorHeap::Init(HeapId heapId, uint32_t capacity)
{
	auto device = dx12Mgr_->GetDevice();
	assert(device);
	assert(heapId != kInvalidHeapId);

	heapId_ = heapId;
	capacity_ = capacity;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = capacity;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_));
	assert(SUCCEEDED(hr));

	incSize_ = device->GetDescriptorHandleIncrementSize(desc.Type);
	cpuStart_ = heap_->GetCPUDescriptorHandleForHeapStart();
	gpuStart_ = heap_->GetGPUDescriptorHandleForHeapStart();
}

void TransientDescriptorHeap::Reset()
{
	used_ = 0;
}

GpuTableHandle TransientDescriptorHeap::AllocTable(uint32_t count)
{
	assert(count > 0);
	assert(used_ + count <= capacity_);

	GpuTableHandle t{};
	t.heapId = heapId_;
	t.count = count;
	t.gpu.ptr = gpuStart_.ptr + static_cast<UINT64>(used_) * incSize_;
	used_ += count;
	return t;
}