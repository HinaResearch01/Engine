#pragma once

#include <cstdint>
#include <cassert>
#include "DescriptorHeap.h"
#include "DescriptorUtils.h"

namespace Tsumi::DX12 {

class TransientDescAllocator {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	TransientDescAllocator() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~TransientDescAllocator() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(DescriptorHeap* heap, uint32_t baseIndex, uint32_t capacity) {
		heap_ = heap;
		base_ = baseIndex;
		capacity_ = capacity;
		cursor_ = 0;

		assert(heap_);
		assert(base_ + capacity_ <= heap_->GetCapacity());
	}

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void Begin() {
		cursor_ = 0;
	}

	/// <summary>
	/// count 個の descriptor を連続確保
	/// </summary>
	DescriptorHandle  Allocate(uint32_t count = 1) {
		if (!heap_ || count == 0) return {};
		if (cursor_ + count > capacity_) return {};

		const uint32_t idx = base_ + cursor_;
		cursor_ += count;
		return heap_->At(idx);
	}

	/// <summary>
	/// 
	/// </summary>
	void Copy(ID3D12Device* device, const DescriptorHandle& dstBase,
			  uint32_t dstOffset, const DescriptorHandle& src) {
		assert(device);
		assert(dstBase.valid());
		assert(src.valid());

		D3D12_CPU_DESCRIPTOR_HANDLE dst = dstBase.cpu;
		dst.ptr += static_cast<SIZE_T>(dstOffset) * heap_->GetDescriptorSize();

		device->CopyDescriptorsSimple(
			1,
			dst,
			src.cpu,
			heap_->GetType()
		);
	}

#pragma region Accessor
	DescriptorHeap* GetHeap() const { return heap_; }
	uint32_t GetBase() const { return base_; }
	uint32_t GetCapacity() const { return capacity_; }
#pragma endregion

private:
	DescriptorHeap* heap_ = nullptr;
	uint32_t base_ = 0;
	uint32_t capacity_ = 0;
	uint32_t cursor_ = 0;
};

}