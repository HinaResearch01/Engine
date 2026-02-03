#pragma once

#include <cstdint>
#include "DX12/Cmd/CommandContext.h"
#include "FrameBindState.h"
#include "FrameUploadArena.h"
#include "DX12/Desc/TransientDescAllocator.h"

namespace Tsumi::DX12 {

/*  */
class FrameResources {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameResources() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameResources() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(ID3D12Device* device, uint32_t uploadSize, DX12::DescriptorHeap* pHeaps, uint32_t transBase, uint32_t transCount) {
		device_ = device;
		upload.Init(device, uploadSize);
		transDescAlloc.Init(pHeaps, transBase, transCount);
	}

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize() {
		upload.Finalize();
	}

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void Begin(DX12::CommandContext& cmd);

	/// <summary>
	/// リセット処理
	/// </summary>
	void Reset() {
		bind.Reset();
	}

	/// <summary>
	/// Root CBV用: データアップロード + ディスクリプタ作成
	/// </summary>
	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadToRootCB(const T& data) {
		return upload.UploadConstAlign(data);
	}

	/// <summary>
	/// Table CBV用: データアップロード + ディスクリプタ作成
	/// </summary>
	template<typename T>
	D3D12_GPU_DESCRIPTOR_HANDLE UploadToTableCB(const T& data) {
		// アップロードしてGPUアドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS va = upload.UploadConstAlign(data);
		uint32_t size = (sizeof(T) + 255) & ~255;

		// ディスクリプタを確保 
		auto handle = transDescAlloc.Allocate(1);

		// ビューを作成 
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
		desc.BufferLocation = va;
		desc.SizeInBytes = size;
		device_->CreateConstantBufferView(&desc, handle.cpu);

		// GPUハンドルを返す 
		return handle.gpu;
	}

public:
	FrameUploadArena upload;
	TransientDescAllocator transDescAlloc;
	FrameBindState bind;
	uint64_t fenceValue = 0;

private:
	ID3D12Device* device_ = nullptr;
};

}