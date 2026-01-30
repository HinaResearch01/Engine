#include "DescriptorHeap.h"
#include <cassert>

using namespace Tsumi::DX12;

void DescriptorHeap::Init(ID3D12Device* device, uint32_t numDescriptors, bool shaderVisible)
{
	capacity_ = numDescriptors;
	D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = type;
	desc.NumDescriptors = numDescriptors;
	desc.Flags = shaderVisible
		? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		: D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_));

	inc_ = device->GetDescriptorHandleIncrementSize(type);
	cpuBase_ = heap_->GetCPUDescriptorHandleForHeapStart();
	gpuBase_ = shaderVisible
		? heap_->GetGPUDescriptorHandleForHeapStart()
		: D3D12_GPU_DESCRIPTOR_HANDLE{};
}

void DescriptorHeap::Finalize()
{
	heap_.Reset();
	inc_ = 0;
	capacity_ = 0;
	cpuBase_.ptr = 0;
	gpuBase_.ptr = 0;
}

DescriptorHandle DescriptorHeap::At(uint32_t index) const
{
	DescriptorHandle h{};
	h.cpu.ptr = cpuBase_.ptr + static_cast<unsigned long long>(index) * inc_;
	h.gpu.ptr = gpuBase_.ptr + static_cast<unsigned long long>(index) * inc_;
	return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CpuAt(uint32_t index) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE h = cpuBase_;
	h.ptr += static_cast<SIZE_T>(index) * inc_;
	return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GpuAt(uint32_t index) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE h = gpuBase_;
	h.ptr += static_cast<UINT64>(index) * inc_;
	return h;
}
