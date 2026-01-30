#pragma once

#include <vector>
#include <mutex>
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
	void Init(DescriptorHeap* heap, uint32_t baseIndex, uint32_t capacity, uint32_t frameCount) {
		heap_ = heap;
		base_ = baseIndex;
		capacity_ = capacity;
		next_ = 0;

		frameCount_ = frameCount;
		deferred_.clear();
		deferred_.resize(frameCount_);
		free_.clear();

		assert(heap_);
		assert(base_ + capacity_ <= heap_->GetCapacity());
		assert(frameCount_ >= 1);
	}

	/// <summary>
	/// 解放処理
	/// </summary>
	void Shutdown() {
		std::lock_guard lock(mtx_);
		heap_ = nullptr;
		base_ = 0;
		capacity_ = 0;
		next_ = 0;

		frameCount_ = 0;
		free_.clear();
		deferred_.clear();
	}


	/// <summary>
	/// 永続確保
	/// </summary>
	DescriptorHandle Allocate(uint32_t count = 1) {
		std::lock_guard lock(mtx_);
		if (!heap_ || count == 0) return {};

		// まず free-list から
		if (count == 1 && !free_.empty()) {
			uint32_t idx = free_.back();
			free_.pop_back();
			return MakeHandle_(idx);
		}

		// linear
		if (next_ + count > capacity_) {
			return {}; // out of space
		}

		const uint32_t idx = base_ + next_;
		next_ += count;

		return MakeHandle_(idx);
	}

	/// <summary>
	/// 即解放
	/// </summary>
	void Free(DescriptorHandle h, uint32_t count = 1) {
		if (!h.valid() || count == 0) return;

		std::lock_guard lock(mtx_);

		if (count == 1) {
			free_.push_back(h.index);
			return;
		}

		for (uint32_t i = 0; i < count; ++i) {
			free_.push_back(h.index + i);
		}
	}

	/// <summary>
	/// 遅延解放
	/// </summary>
	void DeferFree(DescriptorHandle h, uint32_t frameIndex, uint32_t count = 1) {
		if (!h.valid() || count == 0) return;

		std::lock_guard lock(mtx_);
		if (frameCount_ == 0) return;

		const uint32_t slot = frameIndex % frameCount_;
		deferred_[slot].push_back(Range{ h.index, count });
	}

	/// <summary>
	/// frameIndex の deferred を回収して free-list に戻す
	/// </summary>
	void ReleaseDeferred(uint32_t frameIndex) {
		std::lock_guard lock(mtx_);
		if (frameCount_ == 0) return;

		const uint32_t slot = frameIndex % frameCount_;
		auto& list = deferred_[slot];

		for (auto& r : list) {
			// r.count を free-list に戻す
			for (uint32_t i = 0; i < r.count; ++i) {
				free_.push_back(r.index + i);
			}
		}
		list.clear();
	}

#pragma region Accessor
	DescriptorHeap* GetHeap() const { return heap_; }
	uint32_t GetBase() const { return base_; }
	uint32_t GetCapacity() const { return capacity_; }
#pragma endregion

private:
	DescriptorHandle MakeHandle_(uint32_t index) const {
		return heap_->At(index);
	}

private:
	struct Range { uint32_t index; uint32_t count; };

	DescriptorHeap* heap_ = nullptr;
	uint32_t base_ = 0;
	uint32_t capacity_ = 0;

	// linear
	uint32_t next_ = 0;

	// free-list (rangeじゃなく index で持つ：まずは簡単に)
	std::vector<uint32_t> free_;

	// deferred[frame] = Range
	uint32_t frameCount_ = 0;
	std::vector<std::vector<Range>> deferred_;

	std::mutex mtx_;
};

}