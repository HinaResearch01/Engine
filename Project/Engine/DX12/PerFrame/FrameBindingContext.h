#pragma once

#include <d3d12.h>
#include <cassert>
#include <string_view>
#include "DX12/Desc/DescriptorHandles.h"
#include "DX12/Desc/TransientDescriptorHeap.h"
#include "DX12/Desc/DescriptorCopier.h"
#include "DX12/PerFrame/FrameUploadAllocator.h"

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;
class CommandContext;
class PersistentDescriptorAllocator;

/* フレームの transient 資源を束ねる唯一の操作点 */
class FrameBindingContext {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameBindingContext() = default;
	FrameBindingContext(DX12Manager* ptr);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameBindingContext() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// フレーム終了時処理
	/// </summary>
	void EndFrame();

	/// <summary>
	/// transient table確保
	/// </summary>
	GpuTableHandle AllocTable(uint32_t count, std::string_view tag = {});

	/// <summary>
	/// CPU descriptors を transient table へコピーして返す
	/// </summary>
	GpuTableHandle BuildTableFromCpuDescs(const CpuDescHandle* src, 
										  uint32_t count, DescriptorCopier& copier, std::string_view tag = {});

	/// <summary>
	/// CB upload
	/// </summary>
	template<class T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadCB(const T& data)
	{
		assert(active_);
		return upload_->UploadCB(data);
	}

	/// <summary>
	/// デバッグ用
	/// </summary>
	uint32_t TransientUsed() const;
	uint32_t TransientCapacity() const;

#pragma region Accessor
	bool IsActive() const { return active_; }
	HeapId GetHeapId() const { return heapId_; }
#pragma endregion

private:
	void BindTransientHeap();

private:
	HeapId heapId_ = kInvalidHeapId;
	bool active_ = false;

	DX12Manager* dx12Mgr_ = nullptr;
	ID3D12Device* device_ = nullptr;
	CommandContext* cmd_ = nullptr;
	TransientDescriptorHeap* heap_ = nullptr;
	FrameUploadAllocator* upload_ = nullptr;
};

}
