#pragma once

#include <d3d12.h>
#include <cstdint>

namespace Tsumi::DX12 {

/// CPU/GPU handle のペア
struct DescriptorHandlePair {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
};

/// ハンドルをオフセットする
inline DescriptorHandlePair OffsetHandle(
	const DescriptorHandlePair& base,
	uint32_t index,
	uint32_t increment)
{
	DescriptorHandlePair h;
	h.cpu.ptr = base.cpu.ptr + index * increment;
	h.gpu.ptr = base.gpu.ptr + index * increment;
	return h;
}

/// Descriptor をコピー（主に Persistent → Transient）
inline void CopyDescriptor(
	ID3D12Device* device,
	const DescriptorHandlePair& dst,
	const DescriptorHandlePair& src,
	UINT count = 1)
{
	device->CopyDescriptorsSimple(
		count,
		dst.cpu,
		src.cpu,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

}
