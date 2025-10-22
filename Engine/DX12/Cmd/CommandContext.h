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

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;		   // コマンドキュー
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_; // コマンドアロケータ
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_;   // コマンドリスト

	DX12Manager* dx12Mgr_ = nullptr;
};

}