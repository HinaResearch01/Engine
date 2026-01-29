#pragma once

#include <d3d12.h>
#include <memory>
#include "DX12/PerFrame/FrameUploadAllocator.h"
#include "DX12/Desc/PerFrame/PerFrameDescriptorAllocator.h"
#include "DX12/Desc/PerFrame/TransientTableBuilder.h"

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

/*  */
class PerFrameResource {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	PerFrameResource() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~PerFrameResource() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	HRESULT Init(ID3D12Device* device, size_t uploadSize);

	/// <summary>
	/// Descriptorの初期化
	/// </summary>
	void InitDescriptors(GlobalDescriptorHeap& globalHeap, 
						 uint32_t frameCount, uint32_t tableBase, uint32_t tableCapPerFrame);

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void BeginFrame(uint32_t frameIndex);

#pragma region Accessor
	FrameUploadAllocator& GetUpload() { return upload_; }
	TransientTableBuilder& GetTableBuilder() { return tableBuilder_; }
	PerFrameDescriptorAllocator& GetTableAllocator() { return tableAlloc_; }
#pragma endregion

private:
	FrameUploadAllocator upload_;
	PerFrameDescriptorAllocator tableAlloc_;
	TransientTableBuilder tableBuilder_;

	ID3D12Device* device_ = nullptr;
};

}