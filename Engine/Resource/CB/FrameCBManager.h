#pragma once

#include <memory>
#include "DynamicCBAllocator.h"

namespace Tsumi::Resource {

class FrameCBManager {

private: // シングルトン
	FrameCBManager()
	{
		cbAllocator_ = std::make_unique<DynamicCBAllocator>();
	}
	~FrameCBManager() = default;
	FrameCBManager(const FrameCBManager&) = delete;
	const FrameCBManager& operator=(const FrameCBManager&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static FrameCBManager* GetInstance() {
		static FrameCBManager instance;
		return &instance;
	}

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
	/// 
	/// </summary>
	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadCB(const T& data)
	{
		assert(cbAllocator_);
		return cbAllocator_->Allocate(&data, sizeof(T));
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
