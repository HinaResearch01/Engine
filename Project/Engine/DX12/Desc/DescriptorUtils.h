#pragma once

#include <d3d12.h>
#include <cstdint>

namespace Tsumi::DX12 {

struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{ 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{ 0 };
	uint32_t index = 0xFFFFFFFFu;

	bool IsValid() const { return cpu.ptr != 0; }
};

struct DescriptorRange {
	DescriptorHandle start{};
	uint32_t count = 0;

	bool IsValid() const { return start.IsValid() && count > 0; }
};

inline D3D12_CPU_DESCRIPTOR_HANDLE Offset(D3D12_CPU_DESCRIPTOR_HANDLE h, uint32_t offset, uint32_t inc) {
	h.ptr += static_cast<SIZE_T>(offset) * inc;
	return h;
}

inline D3D12_GPU_DESCRIPTOR_HANDLE Offset(D3D12_GPU_DESCRIPTOR_HANDLE h, uint32_t offset, uint32_t inc) {
	h.ptr += static_cast<UINT64>(offset) * inc;
	return h;
}

}