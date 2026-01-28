#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include "DescriptorHandles.h"

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

/* フレーム用のヒープ */
class TransientDescriptorHeap {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	TransientDescriptorHeap() = default;
	TransientDescriptorHeap(DX12Manager* ptr);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~TransientDescriptorHeap() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(HeapId heapId, uint32_t capacity);

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

	/// <summary>
	/// GPUTableの割り当て
	/// </summary>
	GpuTableHandle AllocTable(uint32_t count);

	/// <summary>
	/// 
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE CpuAt(const GpuTableHandle& table, uint32_t offset);

#pragma region Accessor
	ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }
	HeapId GetHeapId() const { return heapId_; }
	uint32_t Used() const { return used_; }
	uint32_t Capacity() const { return capacity_; }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
	HeapId heapId_ = kInvalidHeapId;

	uint32_t capacity_ = 0;
	uint32_t used_ = 0;
	uint32_t incSize_ = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuStart_{ 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart_{ 0 };

	DX12Manager* dx12Mgr_ = nullptr;
};

}
