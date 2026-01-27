#include "PersistentDescriptorHeap.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;

PersistentDescriptorAllocator::PersistentDescriptorAllocator(DX12Manager* ptr)
	: dx12Mgr_(ptr)
{}

void PersistentDescriptorAllocator::Init(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity)
{
	auto device = dx12Mgr_->GetDevice();
	assert(device);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = type;
	desc.NumDescriptors = capacity;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU-only
	desc.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_));
	assert(SUCCEEDED(hr));

	capacity_ = capacity;
	used_ = 0;
	incSize_ = device->GetDescriptorHandleIncrementSize(type);
	cpuStart_ = heap_->GetCPUDescriptorHandleForHeapStart();
}

CpuDescHandle PersistentDescriptorAllocator::Allocate()
{
	assert(used_ < capacity_);

	CpuDescHandle h{};
	h.cpu = cpuStart_;
	h.cpu.ptr += static_cast<SIZE_T>(used_) * incSize_;
	++used_;
	return h;
}