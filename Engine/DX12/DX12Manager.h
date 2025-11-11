#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include "Cmd/CommandContext.h"
#include "Desc/DescriptorAllocator.h"
#include "Device/DX12Device.h"
#include "SwapChain/SwapChain.h"
#include "Framebuf/Framebuffer.h"
#include "FrameSync/FrameSync.h"
#include "Utils/DxException/DxException.h"

namespace Tsumi::DX12 {

class DX12Manager {

private: // シングルトン
	DX12Manager();
	~DX12Manager() = default;
	DX12Manager(const DX12Manager&) = delete;
	const DX12Manager& operator=(const DX12Manager&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static DX12Manager* GetInstance() {
		static DX12Manager instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 解放処理
	/// </summary>
	void OnFinalize();

	/// <summary>
	/// フレーム開始処理
	/// </summary>
	HRESULT StartFrame();

	/// <summary>
	/// フレーム終了処理
	/// </summary>
	HRESULT EndFrame();

	/// <summary>
	/// 描画前処理 PostEffect
	/// </summary>
	void PreDraw4PE();

	/// <summary>
	/// 描画後処理 PostEffect
	/// </summary>
	void PostDraw4PE();

	/// <summary>
	/// 描画前処理 SwapChain
	/// </summary>
	void PreDraw4SC();

	/// <summary>
	/// 描画後処理 SwapChain
	/// </summary>
	void PostDraw4SC();


#pragma region Accessor

	ID3D12Device* GetDevice() const {
		return dx12Device_ ? dx12Device_->GetDevice() : nullptr;
	}
	IDXGIFactory7* GetFactory() const {
		return dx12Device_ ? dx12Device_->GetFactory() : nullptr;
	}
	ID3D12CommandQueue* const GetCmdQueue() { 
		return cmdContext_ ? cmdContext_->GetQueue() : nullptr;
	}
	ID3D12CommandAllocator* GetCurrentCmdAllocator() const {
		return cmdContext_ ? cmdContext_->GetCurrentAllocator() : nullptr;
	}
	ID3D12GraphicsCommandList* GetCmdList() const { 
		return cmdContext_ ? cmdContext_->GetList() : nullptr;
	}
	IDXGISwapChain4* GetSwapChain() const {
		return swapChain_ ? swapChain_->GetSwapChain() : nullptr;
	}
	UINT GetBufferCount() const { return bufferCount_; }
	void SetBufferCount(UINT c) { bufferCount_ = (c >= 2) ? c : 2; } // 最小 2 を保証

	CommandContext* GetCommandContext() const { return cmdContext_.get(); }
	DescriptorAllocator* GetTransientDescAlloc() const { return transientDescAlloc_.get(); }
	DescriptorAllocator* GetPersistentDescAlloc() const { return persistentDescAlloc_.get(); }
	Framebuffer* GetFramebuffer() const { return framebuf_.get(); }
	FrameSync* GetFrameSync() const { return frameSync_.get(); }

#pragma endregion

private:

	void PrepareBackBuffer(UINT currIndex, ID3D12Resource* backBuffer);
	void BindRenderTargets(UINT currIndex);
	void ClearRenderTargets(UINT currIndex);
	void TransitionToPresent(UINT currIndex, ID3D12Resource* backBuffer);

private:
	std::unique_ptr<CommandContext> cmdContext_;
	std::unique_ptr<DescriptorAllocator> transientDescAlloc_;
	std::unique_ptr<DescriptorAllocator> persistentDescAlloc_;
	std::unique_ptr<DX12Device> dx12Device_;
	std::unique_ptr<SwapChain> swapChain_;
	std::unique_ptr<Framebuffer> framebuf_;
	std::unique_ptr<FrameSync> frameSync_;

	// デフォルトはトリプルバッファ
	UINT bufferCount_ = 3;
};

}