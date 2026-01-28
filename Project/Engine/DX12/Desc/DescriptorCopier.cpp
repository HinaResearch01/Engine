#include "DescriptorCopier.h"

using namespace Tsumi::DX12;

void DescriptorCopier::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	device_ = device;
	type_ = type;
}

void Tsumi::DX12::DescriptorCopier::Copy1(const CpuDescHandle& src, D3D12_CPU_DESCRIPTOR_HANDLE dst)
{
	assert(device_);
	assert(src.IsValid());
	device_->CopyDescriptorsSimple(1, dst, src.cpu, type_);
}