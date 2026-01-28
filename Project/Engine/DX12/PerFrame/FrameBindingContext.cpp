#include "FrameBindingContext.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;

FrameBindingContext::FrameBindingContext(DX12Manager* ptr)
	: dx12Mgr_(ptr)
{}

void FrameBindingContext::Init()
{
	device_ = dx12Mgr_->GetDevice();
	cmd_ = dx12Mgr_->GetCommandContext();
	//heap_ = dx12Mgr_->GetTransDescHeap();
	//upload_ = dx12Mgr_->GetFrameUploadAlloc();
}

void FrameBindingContext::BeginFrame()
{
	assert(!active_);
	assert(device_);

	heapId_ = heap_->GetHeapId();
	assert(heapId_ != kInvalidHeapId);

	heap_->Reset();
	upload_->Reset();
	BindTransientHeap();

	active_ = true;
}

void FrameBindingContext::EndFrame()
{
	assert(active_);
	active_ = false;

	device_ = nullptr;
	cmd_ = nullptr;
	heap_ = nullptr;
	upload_ = nullptr;
	heapId_ = kInvalidHeapId;
}

GpuTableHandle FrameBindingContext::AllocTable(uint32_t count, std::string_view tag)
{
	assert(active_);
	return heap_->AllocTable(count);
}

GpuTableHandle FrameBindingContext::BuildTableFromCpuDescs(const CpuDescHandle* src, uint32_t count, DescriptorCopier& copier, std::string_view tag)
{
	assert(active_);
	assert(src && count > 0);

	auto table = heap_->AllocTable(count);

	for (uint32_t i = 0; i < count; ++i) {
		assert(src[i].IsValid());
		auto dstCpu = heap_->CpuAt(table, i);
		copier.Copy1(src[i], dstCpu);
	}

	return table;
}

uint32_t FrameBindingContext::TransientUsed() const
{
	assert(heap_);
	return heap_->Used();
}

uint32_t FrameBindingContext::TransientCapacity() const
{
	assert(heap_);
	return heap_->Capacity();
}

void FrameBindingContext::BindTransientHeap()
{
	assert(cmd_);
	assert(heap_);

	ID3D12DescriptorHeap* heaps[] = { heap_->GetHeap() };
	cmd_->SetDescriptorHeaps(1, heaps, heapId_);
}
