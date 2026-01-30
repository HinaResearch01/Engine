#pragma once

#include <d3d12.h>
#include <cstdint>

namespace Tsumi::DX12 {

struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
};

inline DescriptorHandle OffsetHandle(const DescriptorHandle& base, uint32_t index, uint32_t inc)
{
	DescriptorHandle h{};
	h.cpu.ptr = base.cpu.ptr + static_cast<SIZE_T>(index) * inc;
	h.gpu.ptr = base.gpu.ptr + static_cast<UINT64>(index) * inc;
	return h;
}

// CBV/SRV/UAV heap の descriptor を Copy
inline void CopyDescriptor(
	ID3D12Device* device,
	const DescriptorHandle& dst,
	const DescriptorHandle& src,
	UINT count = 1)
{
	device->CopyDescriptorsSimple(
		count,
		dst.cpu,
		src.cpu,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

}
