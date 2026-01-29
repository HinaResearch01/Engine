#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include <assert.h>

namespace Tsumi::DX12 {

// Viewport / Scissor
struct Viewport {
	float TopLeftX = 0.f;
	float TopLeftY = 0.f;
	float Width = 0.f;
	float Height = 0.f;
	float MinDepth = 0.f;
	float MaxDepth = 1.f;

	D3D12_VIEWPORT ToD3D() const {
		D3D12_VIEWPORT vp{};
		vp.TopLeftX = TopLeftX;
		vp.TopLeftY = TopLeftY;
		vp.Width = Width;
		vp.Height = Height;
		vp.MinDepth = MinDepth;
		vp.MaxDepth = MaxDepth;
		return vp;
	}
	bool operator==(const Viewport& o) const {
		return TopLeftX == o.TopLeftX && TopLeftY == o.TopLeftY && Width == o.Width && Height == o.Height
			&& MinDepth == o.MinDepth && MaxDepth == o.MaxDepth;
	}
	bool operator!=(const Viewport& o) const { return !(*this == o); }
};

struct Scissor {
	LONG Left = 0;
	LONG Top = 0;
	LONG Right = 0;
	LONG Bottom = 0;

	D3D12_RECT ToD3D() const { return { Left, Top, Right, Bottom }; }
	bool operator==(const Scissor& o) const {
		return Left == o.Left && Top == o.Top && Right == o.Right && Bottom == o.Bottom;
	}
	bool operator!=(const Scissor& o) const { return !(*this == o); }
};


// 前方宣言
class DX12Manager;

/* Command関係の操作クラス */
class CommandContext {

public:
	/// <summary>
	/// コンストラクタ
	/// コピー禁止
	/// </summary>
	CommandContext() = default;
	CommandContext(DX12Manager* ptr,
				   D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	CommandContext(const CommandContext&) = delete;
	CommandContext& operator=(const CommandContext&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~CommandContext();

	/// <summary>
	/// 明示的にフレーム数を設定する（Create の前に呼ぶ）
	/// </summary>
	void SetFrameCount(UINT frameCount) { frameCount_ = (frameCount >= 2) ? frameCount : 2; }

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

	/// <summary>
	/// ビューポート設定
	/// </summary>
	void SetViewport(const Viewport& vp);

	/// <summary>
	/// シザー矩形設定
	/// </summary>
	void SetScissor(const Scissor& sc);

	/// <summary>
	/// SwapChainサイズやレンダーターゲットに合わせて、
	/// 自動的にフルスクリーン範囲を設定する際に使用。
	/// </summary>
	void SetFullViewportFromFramebuffer();
	void SetFullScissorFromFramebuffer();

	/// <summary>
	/// descriptor heap binding
	/// </summary>
	void SetDescriptorHeaps(uint32_t count, ID3D12DescriptorHeap* const* heaps);

	/// <summary>
	/// Root descriptor table
	/// </summary>
	void SetGraphicsRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE table);

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

	/// <summary>
	/// viewportとscissorのリセット
	/// </summary>
	void ResetCachedRasterState();

public:

#pragma region Accessor
	ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }
	ID3D12CommandAllocator* GetCurrentAllocator() const {
		return (currentFrameIndex_ < allocators_.size()) ? allocators_[currentFrameIndex_].Get() : nullptr;
	}
	ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }
	D3D12_COMMAND_LIST_TYPE GetListType() const { return listType_; }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> allocators_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_;

	// fence sync
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	HANDLE fenceEvent_ = nullptr;
	UINT64 globalFenceValue_ = 0;
	std::vector<UINT64> fenceValues_;

	DX12Manager* dx12Mgr_ = nullptr;
	UINT frameCount_ = 2;
	UINT currentFrameIndex_ = 0;

	// cached raster state
	bool viewportSet_ = false;
	Viewport currentViewport_{};
	bool scissorSet_ = false;
	Scissor currentScissor_{};

	// command list open/closed
	bool isListOpen_ = false;

	D3D12_COMMAND_LIST_TYPE listType_ = D3D12_COMMAND_LIST_TYPE_DIRECT;
};

}