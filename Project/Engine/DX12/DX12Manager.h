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
#include "PerFrame/FrameResources.h"

namespace Tsumi::DX12 {


struct FrameIndices {
	uint32_t cpu = 0;
	uint32_t backBuffer = 0; 
};

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
	FrameIndices BeginFrame();
	HRESULT EndFrame(const FrameIndices& idx);

	// ---- render passes ----
	void BeginGBufferPass();
	void ClearGBuffer();
	void BeginBackBufferPass(uint32_t backBufferIndex);
	void ClearBackBuffer(uint32_t backBufferIndex);

	// ---- Wait For GPU ----
	void WaitForGpu();

	// ---- view state ----
	D3D12_VIEWPORT GetMainViewport() const;
	D3D12_RECT     GetMainScissor() const;

	// ---- Transition GBuffer ----
	void TransitionGBufferToWrite();
	void TransitionGBufferToRead();

private:

	void InitDescriptors();
	void InitFrames();
	
	// ---- helpers ----
	void PrepareBackBuffer(UINT index);
	void BindBackBuffer(UINT index);
	void TransitionToPresent(UINT index);

public:
#pragma region Accessors
	// Device / Factory / Command
	ID3D12Device* GetDevice() const {
		return device_ ? device_->GetDevice() : nullptr;
	}
	IDXGIFactory7* GetFactory() const {
		return device_ ? device_->GetFactory() : nullptr;
	}
	CommandContext* GetCommandContext() const { return graphicsCtx_.get(); }
	ID3D12GraphicsCommandList* GetCmdList() const {
		return graphicsCtx_ ? graphicsCtx_->GetList() : nullptr;
	}
	ID3D12CommandQueue* GetGraphicsQueue() const {
		return graphicsCtx_ ? graphicsCtx_->GetQueue() : nullptr;
	}
	CommandContext* GetUploadCmdContext() const { return uploadCtx_.get(); }
	CommandContext* GetResourceCmdContext() const { return resourceCtx_.get(); }

	// SwapChain
	SwapChain* GetSwapChain() const { return swapChain_.get(); }
	IDXGISwapChain4* GetIDXGISwapChain4() const {
		return swapChain_ ? swapChain_->GetSwapChain4() : nullptr;
	}
	uint32_t GetCurrentBackBufferIndex() const {
		return swapChain_ ? swapChain_->GetCurrentBackBufferIndex() : 0;
	}
	uint32_t GetDesiredBufferCount() const { return desiredBufferCount_; }
	uint32_t GetBufferCount() const { return bufferCount_; }
	DXGI_FORMAT GetBackBufferFormat() const {
		return swapChain_ ? swapChain_->GetDesc().Format : DXGI_FORMAT_UNKNOWN;
	}

	// Descriptor
	ID3D12DescriptorHeap* GetGlobalDescriptorHeap() const { return descHeap_.GetHeap(); }
	uint32_t GetGlobalDescriptorStride() const { return descHeap_.GetDescriptorSize(); }
	PersistentDescAllocator* GetPersistentDescAllocator() { return &perDescAlloc_; }

	// Framebuffer
	Framebuffer* GetFramebuffer() const { return framebuffer_.get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGBufferSrvTable() const {
		return framebuffer_->GetGBufferSrvBase().gpu;
	}

	// Frame Sync
	FrameSync* GetFrameSync() const { return frameSync_.get(); }

	// 変更点：FrameResources は cpu index で取る（混線APIを排除）
	FrameResources& GetFrameResource(uint32_t cpuFrameIndex) {
		assert(cpuFrameIndex < frames_.size());
		return frames_[cpuFrameIndex];
	}
#pragma endregion

private:
	std::unique_ptr<DX12Device> device_;
	std::unique_ptr<CommandContext> graphicsCtx_;
	std::unique_ptr<CommandContext> uploadCtx_;
	std::unique_ptr<CommandContext> resourceCtx_;

	DescriptorHeap descHeap_;
	PersistentDescAllocator perDescAlloc_;

	std::unique_ptr<SwapChain> swapChain_;
	std::unique_ptr<Framebuffer> framebuffer_;
	std::unique_ptr<FrameSync> frameSync_;

	std::vector<FrameResources> frames_;

	// parameters
	uint32_t desiredBufferCount_ = 3;
	uint32_t bufferCount_ = 3;

	uint32_t backBufferIndex_ = 0;
	uint32_t cpuFrameIndex_ = 0;

	uint32_t totalDescriptors_ = 65536;
	uint32_t persistentCap_ = 32768;
	uint32_t transientCapPerFrame_ = 8192;
	uint32_t uploadBytesPerFrame_ = 16 * 1024;

};

} 
