#include "PerFrameDescriptorAllocator.h"

using namespace Tsumi::DX12;

void PerFrameDescriptorAllocator::Init(GlobalDescriptorHeap& heap, uint32_t frameCount, uint32_t baseIndex, uint32_t capacityPerFrame)
{
	heap_ = &heap;
	frameCount_ = frameCount;
	base_ = baseIndex;
	capPerFrame_ = capacityPerFrame;
	frameIndex_ = 0;
	cursor_ = 0;
}

void PerFrameDescriptorAllocator::BeginFrame(uint32_t frameIndex)
{
	frameIndex_ = frameIndex % frameCount_;
	cursor_ = 0;
}

DescriptorRange PerFrameDescriptorAllocator::Allocate(uint32_t count)
{
	DescriptorRange r{};
	if (!heap_ || count == 0) return r;
	if (cursor_ + count > capPerFrame_) return r;

	const uint32_t globalStart = base_ + frameIndex_ * capPerFrame_ + cursor_;
	cursor_ += count;

	r.start = heap_->At(globalStart);
	r.count = count;
	return r;
}
