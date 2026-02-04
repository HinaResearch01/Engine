#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <array>
#include <cassert>
#include <vector>

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

/* GPUとCPUのフレーム同期を管理するクラス */
class FrameSync {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameSync() = default;
	FrameSync(DX12Manager* ptr);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameSync();

	/// <summary>
	/// 初期化処理
	/// </summary>
	HRESULT Init();

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void BeginFrame(uint32_t frameIndex);

	/// <summary>
	/// フレーム終了時処理
	/// </summary>
	void  EndFrame(uint32_t frameIndex);

	/// <summary>
	/// 明示待ち
	/// </summary>
	void Wait(uint64_t fenceValue);

#pragma region Accessor
	uint32_t GetFrameIndex() const { return frameIndex_; }
#pragma endregion 

public:
	static constexpr uint32_t kFrameCount = 3;

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	HANDLE fenceEvent_ = nullptr;

	uint32_t bufferCount_ = 0;
	uint32_t frameIndex_ = 0;

	std::vector<uint64_t> frameFenceValues_;
	uint64_t nextFenceValue_ = 0;

	DX12Manager* dx12Mgr_ = nullptr;
};

}
