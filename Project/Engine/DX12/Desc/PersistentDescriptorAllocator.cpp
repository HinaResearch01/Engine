#include "PersistentDescriptorAllocator.h"

using namespace Tsumi::DX12;

void PersistentDescriptorAllocator::Init(GlobalDescriptorHeap& heap, uint32_t baseIndex, uint32_t capacity)
{
	heap_ = &heap;
	base_ = baseIndex;
	cap_ = capacity;

	freeList_.clear();
	freeList_.reserve(capacity);
	// 末尾からpopする
	for (uint32_t i = 0; i < capacity; ++i) {
		freeList_.push_back(baseIndex + (capacity - 1 - i));
	}
}

void PersistentDescriptorAllocator::Finalize()
{
	freeList_.clear();
	heap_ = nullptr;
	base_ = 0;
	cap_ = 0;
}

DescriptorHandle PersistentDescriptorAllocator::Allocate()
{
	DescriptorHandle h{};
	if (!heap_ || freeList_.empty()) return h;

	const uint32_t idx = freeList_.back();
	freeList_.pop_back();
	return heap_->At(idx);
}

void PersistentDescriptorAllocator::Free(const DescriptorHandle& h)
{
	if (!heap_ || !h.IsValid()) return;
	// 範囲チェック
	if (h.index < base_ || h.index >= base_ + cap_) return;
	freeList_.push_back(h.index);
}

DescriptorHandle PersistentDescriptorAllocator::AllocateAndCopyFromCpu(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srcCpu)
{
	auto dst = Allocate();
	if (!dst.IsValid() || !device || srcCpu.ptr == 0) return {};

	device->CopyDescriptorsSimple(
		1, dst.cpu, srcCpu,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return dst;
}

DescriptorHandle PersistentDescriptorAllocator::At(uint32_t localIndex) const
{
	if (!heap_ || localIndex >= cap_) return {};
	return heap_->At(base_ + localIndex);
}
