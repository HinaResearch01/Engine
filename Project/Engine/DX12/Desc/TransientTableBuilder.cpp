#include "TransientTableBuilder.h"

using namespace Tsumi::DX12;

void TransientTableBuilder::Init(PerFrameDescriptorAllocator& tableAlloc, ID3D12Device* device)
{
	alloc_ = &tableAlloc;
	device_ = device;
}

void TransientTableBuilder::BeginFrame(uint32_t frameIndex)
{
	if (alloc_) alloc_->BeginFrame(frameIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE TransientTableBuilder::BuildTable(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> srcCpu)
{
	D3D12_GPU_DESCRIPTOR_HANDLE nullGpu{ 0 };
	if (!alloc_ || !device_ || srcCpu.empty()) return nullGpu;

	auto range = alloc_->Allocate(static_cast<uint32_t>(srcCpu.size()));
	if (!range.IsValid()) return nullGpu;

	const UINT inc = alloc_->GetDescriptorSize();
	DescriptorCopier::CopyMany(device_, range.start.cpu, inc, srcCpu);

	return range.start.gpu;
}
