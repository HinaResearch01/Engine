#pragma once

#include <cstdint>
#include <cassert>
#include "DescriptorUtils.h"
#include "DescriptorHeap.h"

namespace Tsumi::DX12 {

class PersistentDescAllocator {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	PersistentDescAllocator() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~PersistentDescAllocator() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(DescriptorHeap* heap, uint32_t capacity) {
		heap_ = heap;
		capacity_ = capacity;
		offset_ = 0;
		assert(heap_);
		assert(capacity_ <= heap_->GetCapacity());
	}

	/// <summary>
	/// 永続確保
	/// </summary>
	uint32_t Allocate(uint32_t count = 1) {
		assert(heap_);
		assert(offset_ + count <= capacity_);
		const uint32_t base = offset_;
		offset_ += count;
		return base;
	}

	/// <summary>
	/// 
	/// </summary>
	DescriptorHandle At(uint32_t index) const {
		assert(heap_);
		return heap_->At(index);
	}

	/// <summary>
	/// 
	/// </summary>
	DescriptorHandle AllocateHandle(uint32_t count = 1) {
		return At(Allocate(count));
	}

#pragma region Accessor
	uint32_t Used() const { return offset_; }
	uint32_t Capacity() const { return capacity_; }
	uint32_t Increment() const { return heap_ ? heap_->GetDescriptorSize() : 0; }
#pragma endregion

private:
	DescriptorHeap* heap_ = nullptr;
	uint32_t capacity_ = 0;
	uint32_t offset_ = 0;
};

}