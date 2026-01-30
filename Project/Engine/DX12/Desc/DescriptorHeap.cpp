#include "DescriptorHeap.h"
#include <cassert>

using namespace Tsumi::DX12;

void DescriptorHeap::Init(ID3D12Device* device, uint32_t numDescriptors, bool shaderVisible)
{
	capacity_ = numDescriptors;
	D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	assert(device);
	type_ = type;
	capacity_ = numDescriptors;
	shaderVisible_ = shaderVisible;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = type;
	desc.NumDescriptors = numDescriptors;
	desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		: D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_));
	assert(SUCCEEDED(hr));

	descriptorSize_ =
		device->GetDescriptorHandleIncrementSize(type_);

	inc_ = device->GetDescriptorHandleIncrementSize(type_);
	cpuBase_ = heap_->GetCPUDescriptorHandleForHeapStart();
	gpuBase_ = shaderVisible ? heap_->GetGPUDescriptorHandleForHeapStart()
		: D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

void DescriptorHeap::Finalize()
{
	heap_.Reset();
	descriptorSize_ = 0;
	inc_ = 0;
	capacity_ = 0;
	shaderVisible_ = false;
	cpuBase_ = { 0 };
	gpuBase_ = { 0 };
}

DescriptorHandle DescriptorHeap::At(uint32_t index) const
{
	DescriptorHandle h{};
	if (!heap_ || index >= capacity_) return h;

	h.index = index;
	h.cpu = CpuAt(index);

	if (shaderVisible_) {
		h.gpu = GpuAt(index);
	}
	return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CpuAt(uint32_t index) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE h = cpuBase_;
	h.ptr += static_cast<SIZE_T>(index) * static_cast<SIZE_T>(inc_);
	return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GpuAt(uint32_t index) const
{
	if (!shaderVisible_) return {};

	D3D12_GPU_DESCRIPTOR_HANDLE h = gpuBase_;
	h.ptr += static_cast<UINT64>(index) * static_cast<UINT64>(inc_);
	return h;
}