#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <mutex>
#include <cstdint>

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

struct DescriptorAllocation {
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	UINT startIndex = UINT_MAX;
	UINT descriptorCount = 0;
	bool valid() const { return cpuHandle.ptr != 0 || gpuHandle.ptr != 0; }
};

/* ディスクリプタアロケータ */
class DescriptorAllocator {

public:

	/// <summary>
	/// コンストラクタ　
	/// </summary>
	DescriptorAllocator() = default;
	DescriptorAllocator(DX12Manager* ptr);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~DescriptorAllocator() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	HRESULT Init(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// count個分の割り当て
	/// </summary>
	DescriptorAllocation Allocate(UINT count = 1);

	/// <summary>
	/// 全割り当てをクリアして再利用可能にする
	/// </summary>
	void Reset();

#pragma region Accessor
	ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }
	UINT GetDescriptorSize() const { return descriptorSize_; }
	UINT GetCapacity() const { return capacity_; }
#pragma endregion 

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
	D3D12_DESCRIPTOR_HEAP_TYPE heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	UINT capacity_ = 0;
	UINT descriptorSize_ = 0;
	bool shaderVisible_ = false;

	// シンプルな空き管理：bool 配列で確保状態を保持
	std::vector<uint8_t> used_; // 0: free, 1: used

	// 排他
	mutable std::mutex mutex_;

	DX12Manager* dx12Mgr_ = nullptr;
};

}