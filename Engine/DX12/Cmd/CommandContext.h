#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

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
	/// 現在のキューでフェンスが到達するまで待つ
	/// </summary>
	HRESULT WaitForGpu();

#pragma region Accessor

	ID3D12CommandQueue* const GetQueue() { return queue_.Get(); }
	ID3D12CommandAllocator* GetAllocator() const { return allocator_.Get(); }
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
	HRESULT CreateAllocator();

	/// <summary>
	/// Listの生成
	/// </summary>
	HRESULT CreateList();

	/// <summary>
	/// フェンスの生成
	/// </summary>
	HRESULT CreateFence();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;		   // コマンドキュー
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_; // コマンドアロケータ
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_;   // コマンドリスト

	// フェンス同期
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	HANDLE fenceEvent_ = nullptr;
	UINT64 fenceValue_ = 0;

	DX12Manager* dx12Mgr_ = nullptr;
};

}