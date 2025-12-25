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
	FrameCBManager() {
		cbAllocator_ = std::make_unique<DynamicCBAllocator>();
	}

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameCBManager() = default;

	/// <summary>
	/// 初期化処理　
	/// </summary>
	void Init()
	{
		cbAllocator_->Init();
	}

	/// <summary>
	/// フレーム切り替え
	/// </summary>
	void BeginFrame(uint32_t frameIndex)
	{
		assert(cbAllocator_);
		cbAllocator_->BeginFrame(frameIndex);
	}

	/// <summary>
	/// 型付きのヘルパ（既存の UploadCB のテンプレート版）
	/// </summary>
	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadCB(const T& data)
	{
		assert(cbAllocator_);
		return cbAllocator_->Allocate(&data, sizeof(T));
	}

	/// <summary>
	/// UploadCB と同時に DescriptorAllocator からディスクリプタを取得して CBV を作成するユーティリティ。
	/// </summary>
	template<typename T>
	Tsumi::DX12::DescAlloc UploadCBAndCreateView(const T& data)
	{
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for GPU upload");

		// 1) Upload data and get GPU address
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = UploadCB(data);
		if (gpuAddr == 0) {
			throw std::runtime_error("FrameCBManager::UploadCBAndCreateView - UploadCB returned 0 GPU address");
		}

		// 2) Allocate a descriptor from DescriptorAllocator
		auto* transientAlloc = Tsumi::DX12::DX12Manager::GetInstance()->GetTransientDescAlloc();
		if (!transientAlloc) {
			throw std::runtime_error("FrameCBManager::UploadCBAndCreateView - transient descriptor allocator is null");
		}

		auto descAlloc = transientAlloc->Allocate(1);
		if (!descAlloc.valid()) {
			throw std::runtime_error("FrameCBManager::UploadCBAndCreateView - Descriptor allocation failed");
		}

		// 3) Create CBV
		ID3D12Device* device = Tsumi::DX12::DX12Manager::GetInstance()->GetDevice();
		if (!device) {
			throw std::runtime_error("FrameCBManager::UploadCBAndCreateView - device is null");
		}

		// Size must be 256-aligned
		UINT sizeInBytes = static_cast<UINT>((sizeof(T) + 255) & ~255u);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = gpuAddr;
		cbvDesc.SizeInBytes = sizeInBytes;

		device->CreateConstantBufferView(&cbvDesc, descAlloc.cpuHandle);

		// Return the DescAlloc (contains cpu/gpu handles)
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
