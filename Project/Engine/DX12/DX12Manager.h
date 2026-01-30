#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <vector>
#include "Cmd/CommandContext.h"
#include "Desc/DescriptorHeap.h"
#include "Desc/PersistentDescAllocator.h"
#include "Device/DX12Device.h"
#include "SwapChain/SwapChain.h"
#include "Framebuf/Framebuffer.h"
#include "FrameSync/FrameSync.h"
#include "PerFrame/FrameContext.h"

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

	// ---- Wait For GPU ----
	void WaitForGpu();

	// ---- view state ----
	D3D12_VIEWPORT GetMainViewport() const;
	D3D12_RECT     GetMainScissor() const;

private:

	void InitDescriptors_();
	void InitFrames_();
	
	// ---- helpers ----
	void PrepareBackBuffer(UINT index);
	void BindBackBuffer(UINT index);
	void TransitionToPresent(UINT index);

public:

#pragma region Accessors
	// Device / Command
	ID3D12Device* GetDevice() const {
		return device_ ? device_->GetDevice() : nullptr;
	}
	CommandContext* GetCommandContext() const {
		return graphicsCtx_.get();
	}
	ID3D12GraphicsCommandList* GetCmdList() const {
		return graphicsCtx_ ? graphicsCtx_->GetList() : nullptr;
	}
	ID3D12CommandQueue* GetGraphicsQueue() const {
		return graphicsCtx_ ? graphicsCtx_->GetQueue() : nullptr;
	}
	CommandContext* GetUploadCmdContext() const {
		return uploadCtx_.get();
	}
	// SwapChain / Frame
	SwapChain* GetSwapChain() const {
		return swapChain_.get();
	}
	uint32_t GetBufferCount() const {
		return bufferCount_;
	}
	uint32_t GetFrameIndex() const {
		return frameSync_ ? frameSync_->GetFrameIndex() : 0;
	}
	DXGI_FORMAT GetBackBufferFormat() const {
		return swapChain_
			? swapChain_->GetDesc().Format
			: DXGI_FORMAT_UNKNOWN;
	}
	// Descriptor
	ID3D12DescriptorHeap* GetGlobalDescriptorHeap() const {
		return descHeap_.GetHeap();
	}
	PersistentDescAllocator* GetPersistentDescAllocator() {
		return &perDescAlloc_;
	}
	TransientDescAllocator* GetTransientDescAllocator() {
		return &frames_[GetFrameIndex()].transDescAlloc_;
	}
	// Per-frame upload
	FrameUploadArena* GetFrameUploadArena() {
		return &frames_[GetFrameIndex()].upload;
	}
	// Framebuffer
	Framebuffer* GetFramebuffer() const {
		return framebuffer_.get();
	}
#pragma endregion

private:
	std::unique_ptr<DX12Device> device_;
	std::unique_ptr<CommandContext> graphicsCtx_;
	std::unique_ptr<CommandContext> uploadCtx_;

	DescriptorHeap descHeap_;
	PersistentDescAllocator perDescAlloc_;

	std::unique_ptr<SwapChain> swapChain_;
	std::unique_ptr<Framebuffer> framebuffer_;
	std::unique_ptr<FrameSync> frameSync_;

	std::vector<FrameContext> frames_;

	// parameters
	uint32_t bufferCount_ = 3;
	uint32_t frameIndex_ = 0;

	uint32_t totalDescriptors_ = 65536;
	uint32_t persistentCap_ = 32768;
	uint32_t transientCapPerFrame_ = 8192;
	uint32_t uploadBytesPerFrame_ = 16 * 1024;
};

} 
