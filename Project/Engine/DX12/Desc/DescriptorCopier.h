#pragma once

#include <d3d12.h>
#include <cassert>
#include <string_view>
#include "DX12/Desc/DescriptorHandles.h"
#include "DX12/Desc/TransientDescriptorHeap.h"
#include "DX12/PerFrame/FrameUploadAllocator.h"

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

/* persistent -> transient へのコピー担当 */
class DescriptorCopier {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	DescriptorCopier() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~DescriptorCopier() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);

	/// <summary>
	/// コピー処理
	/// </summary>
	void Copy1(const CpuDescHandle& src, D3D12_CPU_DESCRIPTOR_HANDLE dst);

private:
	ID3D12Device* device_ = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE type_{};
};

}
