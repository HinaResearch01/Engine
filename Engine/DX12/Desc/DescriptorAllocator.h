#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <mutex>
#include <cstdint>

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

struct DescAlloc {
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
	HRESULT Init(UINT numDescriptor = 1024, UINT frameBuckets = 3);

	/// <summary>
	/// count個分の割り当て
	/// </summary>
	DescAlloc Allocate(UINT count = 1);

	/// <summary>
	/// 1つの割り当てを解放
	/// </summary>
	void Free(const DescAlloc& handle);

	/// <summary>
	/// 全割り当てをクリアして再利用可能にする
	/// </summary>
	void Reset();

	/// <summary>
	/// 指定の割り当てを「フレームが完了するまで保留」しておく（GPU 側での使用が終わるまで安全に解放しない）
	/// frameIndex: フレームインデックス（例: FrameSync::GetFrameIndex()）
	/// </summary>
	void DeferFree(const DescAlloc& alloc, UINT frameIndex);

	/// <summary>
	/// StartFrame のタイミングで呼ぶ。frameIndex のバケットに登録された割当を実際に解放する。
	/// </summary>
	void CollectDeferred(UINT frameIndex);

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

	// deferred free buckets (frame-based)
	std::vector<std::vector<DescAlloc>> pendingFrees_;

	// 排他
	mutable std::mutex mutex_;

	DX12Manager* dx12Mgr_ = nullptr;
};

}