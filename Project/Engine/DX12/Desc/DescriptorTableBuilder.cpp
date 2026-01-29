#include "DescriptorTableBuilder.h"

using namespace Tsumi::DX12;

DescriptorTableBuilder& DescriptorTableBuilder::AddCopy(DescriptorHandlePair src, uint32_t count)
{
	assert(device_);
	assert(count >= 1);
	Entry e{};
	e.type = EntryType::Copy;
	e.src = src;
	e.count = count;
	entries_.push_back(e);
	return *this;
}

DescriptorTableBuilder& DescriptorTableBuilder::AddCBV(D3D12_GPU_VIRTUAL_ADDRESS gpuVA, uint32_t sizeInBytes)
{
	assert(device_);
	assert(gpuVA != 0);

	// CBV size は 256-byte aligned 必須
	const uint32_t aligned = AlignUp(sizeInBytes, 256);

	Entry e{};
	e.type = EntryType::CBV;
	e.cbvVA = gpuVA;
	e.cbvSize = aligned;
	e.count = 1;
	entries_.push_back(e);
	return *this;
}

DescriptorTable DescriptorTableBuilder::Build()
{
	assert(device_);
	assert(!entries_.empty());

	// まず必要 descriptor 数を数える
	uint32_t total = 0;
	for (const auto& e : entries_) {
		total += e.count;
	}
	assert(total > 0);

	// Transient heap から連続領域を確保
	const uint32_t baseIndex = alloc_.Allocate(total);
	const DescriptorHandlePair base = alloc_.At(baseIndex);

	// 確保した領域に順に書き込む
	uint32_t writeOffset = 0;
	const uint32_t inc = alloc_.At(1).cpu.ptr - alloc_.At(0).cpu.ptr; // heap increment

	for (const auto& e : entries_) {
		const uint32_t dstIndex = baseIndex + writeOffset;
		const DescriptorHandlePair dst = alloc_.At(dstIndex);

		if (e.type == EntryType::Copy) {
			// 連続コピー対応
			CopyDescriptor(device_, dst, e.src, e.count);
			writeOffset += e.count;
		}
		else if (e.type == EntryType::CBV) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
			cbv.BufferLocation = e.cbvVA;
			cbv.SizeInBytes = e.cbvSize;

			device_->CreateConstantBufferView(&cbv, dst.cpu);
			writeOffset += 1;
		}
	}

	DescriptorTable t{};
	t.base = base;
	t.gpu = base.gpu;
	t.baseIndex = baseIndex;
	t.count = total;
	return t;
}
