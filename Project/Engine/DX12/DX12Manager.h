#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <vector>
#include "Cmd/CommandContext.h"
#include "Desc/Global/GlobalDescriptorHeap.h"
#include "Device/DX12Device.h"
#include "SwapChain/SwapChain.h"
#include "Framebuf/Framebuffer.h"
#include "FrameSync/FrameSync.h"
#include "PerFrame/PerFrameResource.h"

namespace Tsumi::DX12 {

class DX12Manager final {

private: // シングルトン
	DX12Manager();
	~DX12Manager() = default;
	DX12Manager(const DX12Manager&) = delete;
	DX12Manager& operator=(const DX12Manager&) = delete;

public:
	// ---- Access Instance ----
	static DX12Manager* GetInstance() {
		static DX12Manager instance;
		return &instance;
	}

	// ---- lifecycle ----
	void Init();
	void Finalize();

	// ---- frame ----
	HRESULT BeginFrame();
	HRESULT EndFrame();

	// ---- render passes ----
	void BeginGBufferPass();
	void ClearGBuffer();

	void BeginBackBufferPass();
	void ClearBackBuffer();

	// ---- view state ----
	D3D12_VIEWPORT GetMainViewport() const;
	D3D12_RECT     GetMainScissor() const;

private:
	
	// ---- helpers ----
	void PrepareBackBuffer(UINT index);
	void BindBackBuffer(UINT index);
	void TransitionToPresent(UINT index);

public:

#pragma region Accessors
	ID3D12Device* GetDevice() const {
		return dx12Device_ ? dx12Device_->GetDevice() : nullptr;
	}
	CommandContext* GetCommandContext() const {
		return graphicsCtx_.get();
	}
	Framebuffer* GetFramebuffer() const {
		return framebuffer_.get();
	}
	FrameSync* GetFrameSync() const {
		return frameSync_.get();
	}
	ID3D12DescriptorHeap* GetGlobalCbvSrvUavHeap() const {
		return GDescHeap_.GetHeap();
	}
#pragma endregion

private:
	std::unique_ptr<DX12Device> dx12Device_;
	std::unique_ptr<CommandContext> graphicsCtx_;
	std::unique_ptr<CommandContext> uploadCtx_;
	GlobalDescriptorHeap GDescHeap_;
	std::unique_ptr<SwapChain> swapChain_;
	std::unique_ptr<Framebuffer> framebuffer_;
	std::unique_ptr<FrameSync> frameSync_;

	std::vector<std::unique_ptr<PerFrameResource>> frameResources_;

	UINT bufferCount_ = 3;
};

} 
