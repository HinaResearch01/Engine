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
	/// 生成
	/// </summary>
	HRESULT Create(UINT frameCount);

	/// <summary>
	/// list を Reset
	/// </summary>
	HRESULT ResetForFrame(UINT frameIndex);

	/// <summary>
	/// Close して Execute
	/// </summary>
	HRESULT Execute();

	/// <summary>
	/// queue flush
	/// </summary>
	HRESULT WaitForGpu();

	/// <summary>
	/// ビューポート / シザー設定
	/// </summary>
	void SetViewport(const Viewport& vp);
	void SetScissor(const Scissor& sc);

	/// <summary>
	/// SwapChainサイズやレンダーターゲットに合わせて、
	/// 自動的にフルスクリーン範囲を設定する際に使用。
	/// </summary>
	void SetFullViewportFromFramebuffer();
	void SetFullScissorFromFramebuffer();

	/// <summary>
	/// 
	/// </summary>
	HRESULT BeginOneShot();          // ResetForFrame(0)
	HRESULT EndOneShot();            // Close + Execute（Waitしない）
	HRESULT EndOneShotAndWait();     // Close + Execute + WaitForGpu


	/// <summary>
	/// thin wrappers
	/// </summary>
	void SetDescriptorHeaps(uint32_t count, ID3D12DescriptorHeap* const* heaps);
	void SetGraphicsRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE table);
	void SetGraphicsRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS va);

private:
	HRESULT CreateQueue();
	HRESULT CreateAllocators(UINT frameCount);
	HRESULT CreateList();

	// flush fence
	HRESULT CreateFlushFence();
	HRESULT SignalFlush();
	HRESULT WaitFlush(uint64_t value);

	void ResetCachedRasterState();

public:

#pragma region Accessor
	ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }
	ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }
	ID3D12CommandAllocator* GetAllocator(UINT frameIndex) const {
		return (frameIndex < allocators_.size()) ? allocators_[frameIndex].Get() : nullptr;
	}
	D3D12_COMMAND_LIST_TYPE GetListType() const { return listType_; }
	bool IsOpen() const { return isListOpen_; }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> allocators_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_;

	uint32_t frameCount_ = 1;
	uint32_t currentFrameIndex_ = 0;

	bool isListOpen_ = false;

	// cached viewport/scissor
	bool viewportSet_ = false;
	bool scissorSet_ = false;
	Viewport currentViewport_{};
	Scissor currentScissor_{};

	// flush fence (WaitForGpu 用)
	Microsoft::WRL::ComPtr<ID3D12Fence> flushFence_;
	HANDLE flushEvent_ = nullptr;
	uint64_t flushValue_ = 0;

	DX12Manager* dx12Mgr_ = nullptr;
	D3D12_COMMAND_LIST_TYPE listType_{};

	uint64_t oneShotFenceValue_ = 0;
};

}