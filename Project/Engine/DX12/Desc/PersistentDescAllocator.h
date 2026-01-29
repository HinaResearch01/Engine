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
	}

	/// <summary>
	/// 永続確保
	/// </summary>
	uint32_t Allocate(uint32_t count = 1) {
		assert(heap_);
		assert(offset_ + count <= capacity_);
		uint32_t base = offset_;
		offset_ += count;
		return base;
	}

	/// <summary>
	/// 
	/// </summary>
	DescriptorHandlePair At(uint32_t index) const {
		return heap_->At(index);
	}

	/// <summary>
	/// 
	/// </summary>
	DescriptorHandlePair AllocateAndGet(uint32_t count = 1) {
		uint32_t index = Allocate(count);
		return heap_->At(index);
	}

#pragma region Accessor
	uint32_t Used() const { return offset_; }
	uint32_t Capacity() const { return capacity_; }
#pragma endregion

private:
	DescriptorHeap* heap_ = nullptr;
	uint32_t capacity_ = 0;
	uint32_t offset_ = 0;
};

}