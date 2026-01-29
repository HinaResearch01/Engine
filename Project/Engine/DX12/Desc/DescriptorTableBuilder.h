#pragma once

#include <d3d12.h>
#include <cstdint>
#include <vector>
#include <cassert>
#include "TransientDescAllocator.h"
#include "DescriptorUtils.h"

namespace Tsumi::DX12 {

struct DescriptorTable {
	DescriptorHandlePair base{};          // base CPU/GPU handle pair
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{};    // base.gpu と同じ（Root に渡す）
	uint32_t baseIndex = 0;
	uint32_t count = 0;

	bool valid() const { return count != 0 && gpu.ptr != 0; }
};

class DescriptorTableBuilder {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	DescriptorTableBuilder() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~DescriptorTableBuilder() = default;

	/// <summary>
	/// リセット
	/// </summary>
	void Reset() { entries_.clear(); }

	/// <summary>
	/// 既存 descriptor をコピー
	/// </summary>
	DescriptorTableBuilder& AddCopy(DescriptorHandlePair src, uint32_t count = 1);

	/// <summary>
	/// CBV をこの table に作る
	/// </summary>
	DescriptorTableBuilder& AddCBV(D3D12_GPU_VIRTUAL_ADDRESS gpuVA, uint32_t sizeInBytes);

	/// <summary>
	/// Table を構築
	/// </summary>
	DescriptorTable Build();

private:
	static uint32_t AlignUp(uint32_t v, uint32_t align) {
		return (v + (align - 1)) & ~(align - 1);
	}

private:

	enum class EntryType : uint8_t {
		Copy,
		CBV
	};

	struct Entry {
		EntryType type{};
		uint32_t count = 1;
		// Copy
		DescriptorHandlePair src{};
		// CBV
		D3D12_GPU_VIRTUAL_ADDRESS cbvVA = 0;
		uint32_t cbvSize = 0;
	};

	ID3D12Device* device_ = nullptr;
	TransientDescAllocator& alloc_;
	std::vector<Entry> entries_;
};

}