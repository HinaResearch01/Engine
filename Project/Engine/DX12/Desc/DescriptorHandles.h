#pragma once
#include <d3d12.h>
#include <cstdint>

namespace Tsumi::DX12 {

using HeapId = uint32_t;
static constexpr HeapId kInvalidHeapId = 0;

// CPU-only（persistent）
struct CpuDescHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{ 0 };
	bool IsValid() const { return cpu.ptr != 0; }
};

// GPU table（transient only）
struct GpuTableHandle {
	HeapId heapId = kInvalidHeapId;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{ 0 };
	uint32_t count = 0;

	bool IsValid() const {
		return heapId != kInvalidHeapId && gpu.ptr != 0;
	}
};

}
