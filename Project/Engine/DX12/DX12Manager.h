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
#include "PerFrame/PerFrameResource.h"
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
	void Finalize();

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

	/// <summary>
	/// Viewportの取得
	/// </summary>
	D3D12_VIEWPORT GetMainViewport() const {
		D3D12_VIEWPORT vp{};
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = static_cast<float>(framebuf_->GetWidth());
		vp.Height = static_cast<float>(framebuf_->GetHeight());
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		return vp;
	}

	/// <summary>
	/// Scissorの取得
	/// </summary>
	D3D12_RECT GetMainScissor() const {
		D3D12_RECT rc{};
		rc.left = 0;
		rc.top = 0;
		rc.right = framebuf_->GetWidth();
		rc.bottom = framebuf_->GetHeight();
		return rc;
	}

	/// <summary>
	/// GBufferSRVTableの取得
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGBufferSRVTable() const {
		return framebuf_->GetGBufferSrvTable();
	}

	/// <summary>
	/// 
	/// </summary>
	void SetGBufferRenderTargets(CommandContext* cmd) {
		auto list = cmd->GetList();
		// RTVs (3)
		D3D12_CPU_DESCRIPTOR_HANDLE rtvs[3] = {
			framebuf_->GetGBufferRtv(0),
			framebuf_->GetGBufferRtv(1),
			framebuf_->GetGBufferRtv(2)
		};
		D3D12_CPU_DESCRIPTOR_HANDLE dsv =
			framebuf_->GetGBufferDsv();

		list->OMSetRenderTargets(3, rtvs, FALSE, &dsv);
	}

	/// <summary>
	/// 
	/// </summary>
	void ClearGBuffer() {
		// cmdContext_ が null だと KernelBase 系で落ちやすいのでガード
		if (!cmdContext_ || !framebuf_) return;
		ID3D12GraphicsCommandList* list = cmdContext_->GetList();
		if (!list) return;
		framebuf_->ClearGBuffer(list);
	}

	/// <summary>
	/// 
	/// </summary>
	void SetBackBufferAsRenderTarget() {
		UINT idx = swapChain_->GetCurrentBackBufferIndex();
		PrepareBackBuffer(idx, framebuf_->GetBackBuffer(idx));
		BindRenderTargets(idx);
	}

	/// <summary>
	/// 
	/// </summary>
	void ClearBackBuffer() {
		UINT idx = swapChain_->GetCurrentBackBufferIndex();
		ClearRenderTargets(idx);
	}

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
	IDXGISwapChain4* GetIDXGISwapChain4() const {
		return swapChain_ ? swapChain_->GetSwapChain() : nullptr;
	}
	UINT GetBufferCount() const { return bufferCount_; }
	void SetBufferCount(UINT c) { bufferCount_ = (c >= 2) ? c : 2; }
	SwapChain* GetSwapChain() const { return swapChain_.get(); }
	CommandContext* GetCommandContext() const { return cmdContext_.get(); }
	CommandContext* GetUploadCmdContext() const { return uploadCmdContext_.get(); }
	DescriptorAllocator* GetTransientDescAlloc() const { return transientDescAlloc_.get(); }
	DescriptorAllocator* GetPersistentDescAlloc() const { return persistentDescAlloc_.get(); }
	Framebuffer* GetFramebuffer() const { return framebuf_.get(); }
	FrameSync* GetFrameSync() const { return frameSync_.get(); }
	PerFrameResource* GetCurrentFrameResource() const {
		uint32_t idx = frameSync_ ? frameSync_->GetFrameIndex() : 0;
		return (idx < frameResources_.size()) ? frameResources_[idx].get() : nullptr;
	}

#pragma endregion

private:

	void PrepareBackBuffer(UINT currIndex, ID3D12Resource* backBuffer);
	void BindRenderTargets(UINT currIndex);
	void ClearRenderTargets(UINT currIndex);
	void TransitionToPresent(UINT currIndex, ID3D12Resource* backBuffer);

private:
	std::unique_ptr<CommandContext> cmdContext_;
	std::unique_ptr<CommandContext> uploadCmdContext_;
	std::unique_ptr<DescriptorAllocator> transientDescAlloc_;
	std::unique_ptr<DescriptorAllocator> persistentDescAlloc_;
	std::unique_ptr<DX12Device> dx12Device_;
	std::unique_ptr<SwapChain> swapChain_;
	std::unique_ptr<Framebuffer> framebuf_;
	std::unique_ptr<FrameSync> frameSync_;
	std::vector<std::unique_ptr<PerFrameResource>> frameResources_;

	// デフォルトはトリプルバッファ
	UINT bufferCount_ = 3;
};

}