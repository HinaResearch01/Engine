#pragma once

#include <d3d12.h>
#include <cstdint>
#include <vector>
#include <cassert>
#include "TransientDescAllocator.h"
#include "DescriptorUtils.h"

namespace Tsumi::DX12 {

struct DescriptorTable {
	DescriptorHandle base{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{}; // base.gpu
	uint32_t baseIndex = 0;
	uint32_t count = 0;

	bool Valid() const { return count > 0 && gpu.ptr != 0; }
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
	DescriptorTableBuilder& AddCopy(DescriptorHandle src, uint32_t count = 1);

	/// <summary>
	/// CBV をこの table に作る
	/// </summary>
	DescriptorTableBuilder& AddCBV(D3D12_GPU_VIRTUAL_ADDRESS gpuVA, uint32_t sizeInBytes);

	/// <summary>
	/// SRV をこの table に作る
	/// </summary>
	DescriptorTableBuilder& AddSRV(ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

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
		CBV,
		SRV
	};

	struct Entry {
		EntryType type{};
		uint32_t count = 1;
		// Copy
		DescriptorHandle src{};
		// CBV
		D3D12_GPU_VIRTUAL_ADDRESS cbvVA = 0;
		uint32_t cbvSize = 0;
		// SRV
		ID3D12Resource* srvRes = nullptr;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	};

	ID3D12Device* device_ = nullptr;
	TransientDescAllocator& alloc_;
	std::vector<Entry> entries_;
};

}