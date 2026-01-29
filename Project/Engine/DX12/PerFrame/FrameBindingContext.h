#pragma once

#include <d3d12.h>
#include <span>
#include "DX12/Cmd/CommandContext.h"
#include "DX12/PerFrame/PerFrameResource.h"
#include "DX12/Desc/Persistent/PersistentDescriptorAllocator.h"

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

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameBindingContext() = default;

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void BeginFrame(CommandContext& cmd, ID3D12DescriptorHeap* globalCbvSrvUavHeap);

	/// <summary>
	/// persistent handle 群を連続table化して返す
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE BuildSrvTable(PerFrameResource& fr,	std::span<const DescriptorHandle> persistentHandles);

	/// <summary>
	/// 
	/// </summary>
	void BindTable(CommandContext& cmd, uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE tableGpu) const;

private:
	bool heapsSet_ = false;
};

}
