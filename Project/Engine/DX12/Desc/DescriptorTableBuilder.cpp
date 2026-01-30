#include "DescriptorTableBuilder.h"

using namespace Tsumi::DX12;

DescriptorTableBuilder& DescriptorTableBuilder::AddCopy(DescriptorHandle src, uint32_t count)
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

	Entry e{};
	e.type = EntryType::CBV;
	e.cbvVA = gpuVA;
	e.cbvSize = AlignUp(sizeInBytes, 256);
	e.count = 1;
	entries_.push_back(e);
	return *this;
}

DescriptorTableBuilder& Tsumi::DX12::DescriptorTableBuilder::AddSRV(ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	assert(device_);
	assert(res != nullptr);

	Entry e{};
	e.type = EntryType::SRV;
	e.srvRes = res;
	e.srvDesc = desc;
	e.count = 1;
	entries_.push_back(e);
	return *this;
}

DescriptorTable DescriptorTableBuilder::Build()
{
	assert(device_);
	assert(!entries_.empty());

	// 必要数を計算
	uint32_t total = 0;
	for (const auto& e : entries_) total += e.count;
	assert(total > 0);

	// 連続領域確保
	const uint32_t baseIndex = alloc_.Allocate(total);
	const DescriptorHandle base = alloc_.At(baseIndex);

	uint32_t write = 0;

	for (const auto& e : entries_) {
		const uint32_t dstIndex = baseIndex + write;
		const DescriptorHandle dst = alloc_.At(dstIndex);

		switch (e.type) {
			case EntryType::Copy:
			CopyDescriptor(device_, dst, e.src, e.count);
			write += e.count;
			break;

			case EntryType::CBV: {
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
				cbv.BufferLocation = e.cbvVA;
				cbv.SizeInBytes = e.cbvSize;
				device_->CreateConstantBufferView(&cbv, dst.cpu);
				write += 1;
				break;
			}

			case EntryType::SRV:
			device_->CreateShaderResourceView(e.srvRes, &e.srvDesc, dst.cpu);
			write += 1;
			break;
		}
	}

	DescriptorTable t{};
	t.base = base;
	t.gpu = base.gpu;
	t.baseIndex = baseIndex;
	t.count = total;
	return t;
}
