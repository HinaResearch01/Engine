#pragma once

#include <span>
#include <d3d12.h>
#include "DX12/Desc/PerFrame/PerFrameDescriptorAllocator.h"
#include "DX12/Desc/Common/DescriptorCopier.h"

namespace Tsumi::DX12 {

class TransientTableBuilder {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	TransientTableBuilder() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~TransientTableBuilder() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(PerFrameDescriptorAllocator& tableAlloc, ID3D12Device* device);

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void BeginFrame(uint32_t frameIndex);

	/// <summary>
	/// CPU handles を連続領域へコピーして、先頭GPU handle を返す
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE BuildTable(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> srcCpu);

private:
	PerFrameDescriptorAllocator* alloc_ = nullptr;
	ID3D12Device* device_ = nullptr;
};

} 
