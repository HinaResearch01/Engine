#pragma once

#include <cstdint>
#include <d3d12.h>
#include "DX12/Desc/Common/DescriptorHandles.h"
#include "DX12/Desc/Global/GlobalDescriptorHeap.h"

namespace Tsumi::DX12 {

/* frame寿命の linear allocator */
class PerFrameDescriptorAllocator {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	PerFrameDescriptorAllocator() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~PerFrameDescriptorAllocator() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(GlobalDescriptorHeap& heap, 
			  uint32_t frameCount, uint32_t baseIndex, uint32_t capacityPerFrame);

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void BeginFrame(uint32_t frameIndex);

	/// <summary>
	/// 割り当て
	/// </summary>
	DescriptorRange Allocate(uint32_t count);

#pragma region Accessor
	uint32_t GetDescriptorSize() const { return heap_ ? heap_->GetDescriptorSize() : 0; }
	uint32_t CapacityPerFrame() const { return capPerFrame_; }
#pragma endregion

private:
	uint32_t frameCount_ = 0;
	uint32_t base_ = 0;
	uint32_t capPerFrame_ = 0;

	uint32_t frameIndex_ = 0;
	uint32_t cursor_ = 0; 

	GlobalDescriptorHeap* heap_ = nullptr;
};

}