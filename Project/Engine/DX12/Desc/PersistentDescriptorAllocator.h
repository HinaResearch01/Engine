#pragma once

#include <vector>
#include <cstdint>
#include <d3d12.h>
#include "DX12/Desc/Common/DescriptorHandles.h"
#include "DX12/Desc/Global/GlobalDescriptorHeap.h"

namespace Tsumi::DX12 {

/*  */
class PersistentDescriptorAllocator {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	PersistentDescriptorAllocator() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~PersistentDescriptorAllocator() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(GlobalDescriptorHeap& heap, uint32_t baseIndex, uint32_t capacity);

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 割り当て
	/// </summary>
	DescriptorHandle Allocate();           // 1個

	/// <summary>
	/// 空
	/// </summary>
	void Free(const DescriptorHandle& h);  // 1個

	/// <summary>
	/// CPUで作ったdescriptorを persistent slot にコピー
	/// </summary>
	DescriptorHandle AllocateAndCopyFromCpu(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srcCpu);

#pragma region Accessor
	DescriptorHandle At(uint32_t localIndex) const; // 0..capacity-1
	uint32_t BaseIndex() const { return base_; }
	uint32_t Capacity() const { return cap_; }
#pragma endregion

private:
	GlobalDescriptorHeap* heap_ = nullptr;
	uint32_t base_ = 0;
	uint32_t cap_ = 0;

	std::vector<uint32_t> freeList_;
};

}