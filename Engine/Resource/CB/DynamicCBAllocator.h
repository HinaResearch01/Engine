#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>
#include <cassert>

namespace Tsumi::DX12 {
class DX12Manager;
}

namespace Tsumi::Resource {

/*  フレームごとに再利用可能な動的定数バッファアロケータ */
class DynamicCBAllocator {

private: // シングルトン
	DynamicCBAllocator();
	~DynamicCBAllocator() = default;
	DynamicCBAllocator(const DynamicCBAllocator&) = delete;
	const DynamicCBAllocator& operator=(const DynamicCBAllocator&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static DynamicCBAllocator* GetInstance() {
		static DynamicCBAllocator instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(uint32_t totalSizePerframe = 3);

	/// <summary>
	/// フレーム切り替え
	/// </summary>
	void BeginFrame(uint32_t frameIndex);

	/// <summary>
	/// 指定サイズの定数バッファ領域を書確保、CPU->GPUへコピー
	/// </summary>
	D3D12_GPU_VIRTUAL_ADDRESS Allocate(const void* srcData, uint32_t size);

#pragma region Accessor
	ID3D12Resource* GetCurrentHeap() const { return uploadHeaps_[currentFrame_].Get(); }
#pragma endregion

private:
	/// <summary>
	/// 256バイトアライメントを保証
	/// </summary>
	uint32_t Align256(uint32_t size)
	{
		return (size + 255) & ~255u;
	}

public:
	static constexpr uint32_t kFrameCount = 3; // triple buffering

private:
	uint32_t totalSizePerFrame_ = 0;
	uint32_t currentOffset_ = 0;
	uint32_t currentFrame_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeaps_[kFrameCount];
	uint8_t* mappedPtrs_[kFrameCount] = { nullptr };

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}