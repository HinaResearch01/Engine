#pragma once

#include <memory>
#include <cassert>
#include "DynamicCBAllocator.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "DX12/DX12Manager.h"

namespace Tsumi::Resource {

class FrameCBManager {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameCBManager()
		: cbAllocator_(std::make_unique<DynamicCBAllocator>())
	{}

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameCBManager() = default;

	/// <summary>
	/// 初期化処理　
	/// </summary>
	void Init() {}

	/// <summary>
	/// フレーム切り替え
	/// </summary>
	void BeginFrame(uint32_t frameIndex)
	{
		frameIndex; // 未使用
		auto* fr = DX12::DX12Manager::GetInstance()->GetCurrentFrameResource();
		assert(fr && "PerFrameResource is null");
		cbAllocator_->Attach(fr);
	}

	/// <summary>
	/// 型付きのヘルパ（既存の UploadCB のテンプレート版）
	/// </summary>
	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadCB(const T& data)
	{
		static_assert(std::is_trivially_copyable_v<T>,
					  "UploadCB requires trivially copyable type");
		assert(cbAllocator_);
		return cbAllocator_->UploadCB(data);
	}

	/// <summary>
	/// UploadCB と同時に DescriptorAllocator からディスクリプタを取得して CBV を作成するユーティリティ。
	/// </summary>
	template<typename T>
	Tsumi::DX12::DescAlloc UploadCBAndCreateView(const T& data)
	{
		static_assert(std::is_trivially_copyable_v<T>,
					  "T must be trivially copyable for GPU upload");

		// 1) Upload data and get GPU address
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = UploadCB(data);
		assert(gpuAddr != 0 && "UploadCB failed");

		// 2) Allocate a descriptor from DescriptorAllocator
		auto* transientAlloc =
			DX12::DX12Manager::GetInstance()->GetTransientDescAlloc();
		assert(transientAlloc && "Transient DescriptorAllocator is null");

		auto descAlloc = transientAlloc->Allocate(1);
		assert(descAlloc.valid() && "Descriptor allocation failed");

		// 3) Create CBV
		ID3D12Device* device =
			DX12::DX12Manager::GetInstance()->GetDevice();
		assert(device && "D3D12 device is null");

		const UINT sizeInBytes =
			static_cast<UINT>((sizeof(T) + 255) & ~255u);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = gpuAddr;
		cbvDesc.SizeInBytes = sizeInBytes;

		device->CreateConstantBufferView(
			&cbvDesc,
			descAlloc.cpuHandle
		);

		return descAlloc;
	}

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize()
	{
		cbAllocator_.reset();
	}

private:
	std::unique_ptr<DynamicCBAllocator> cbAllocator_;
};

}
