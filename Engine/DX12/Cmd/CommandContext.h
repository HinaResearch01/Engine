#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <vector>

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

/* Command関係の操作クラス */
class CommandContext {

public:
	/// <summary>
	/// コンストラクタ
	/// コピー禁止
	/// </summary>
	CommandContext();
	CommandContext(const CommandContext&) = delete;
	CommandContext& operator=(const CommandContext&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~CommandContext();

	/// <summary>
	/// 生成
	/// </summary>
	HRESULT Create();

	/// <summary>
	/// コマンドリストを実行して GPU の完了を待つ（ユーティリティ）
	/// </summary>
	HRESULT ExecuteAndWait();

	/// <summary>
	/// コマンドリストを実行してフェンスにシグナル（非同期）
	/// シグナル後は MoveToNextFrame() を呼んでフレームを進める
	/// </summary>
	HRESULT ExecuteAndSignal();

	/// <summary>
	/// フレームを進める。次のフレームの allocator を再利用する前に必ず呼ぶ
	/// </summary>
	HRESULT MoveToNextFrame();

	/// <summary>
	/// 現在のキューでフェンスが到達するまで待つ
	/// </summary>
	HRESULT WaitForGpu();

#pragma region Accessor

	ID3D12CommandQueue* const GetQueue() { return queue_.Get(); }
	ID3D12CommandAllocator* GetCurrentAllocator() const {
		return (currentFrameIndex_ < allocators_.size()) ? allocators_[currentFrameIndex_].Get() : nullptr;
	}
	ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }

#pragma endregion

private:
	/// <summary>
	/// Queueの生成
	/// </summary>
	HRESULT CreateQueue();

	/// <summary>
	/// Allocatorの生成
	/// </summary>
	HRESULT CreateAllocators(UINT frameCount);

	/// <summary>
	/// Listの生成
	/// </summary>
	HRESULT CreateList();

	/// <summary>
	/// フェンスの生成
	/// </summary>
	HRESULT CreateFence();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> allocators_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_;

	// フェンス同期（グローバルフェンスとフレームごとの期待値）
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	HANDLE fenceEvent_ = nullptr;
	UINT64 globalFenceValue_ = 0; // シグナル時に増やす値
	std::vector<UINT64> fenceValues_; // フレームごとの「このフレームの最後にシグナルした値」

	// フレーム管理
	UINT currentFrameIndex_ = 0;
	UINT frameCount_ = 3; // デフォルト 3 フレーム。必要なら変更可能。

	DX12Manager* dx12Mgr_ = nullptr;
};

}