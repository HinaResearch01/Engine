#include "DX12Manager.h"
#include "Utils/Logger/Logger.h"
#include "Utils/DxException/DxException.h"
#include <chrono>
#include <format>

using namespace Tsumi::DX12;

DX12Manager::DX12Manager()
{
	graphicsCtx_ = std::make_unique<CommandContext>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	uploadCtx_ = std::make_unique<CommandContext>(this, D3D12_COMMAND_LIST_TYPE_COPY);
	dx12Device_ = std::make_unique<DX12Device>();
	swapChain_ = std::make_unique<SwapChain>(this);
	framebuffer_ = std::make_unique<Framebuffer>(this);
	frameSync_ = std::make_unique<FrameSync>(this);
}

void DX12Manager::Init()
{
	try {
		Utils::Exception::DX_CALL(dx12Device_->Create());
		if (graphicsCtx_) graphicsCtx_->SetFrameCount(bufferCount_);
		if (uploadCtx_)   uploadCtx_->SetFrameCount(1);
		Utils::Exception::DX_CALL(graphicsCtx_->Create());
		Utils::Exception::DX_CALL(uploadCtx_->Create());
		Utils::Exception::DX_CALL(swapChain_->Create());
		Utils::Exception::DX_CALL(framebuffer_->Init());
		Utils::Exception::DX_CALL(frameSync_->Init());

		// descriptor heap
		constexpr uint32_t kTotalDescriptors = 65536;
		descHeap_.Init(
			GetDevice(),
			kTotalDescriptors,
			true
		);

		// Persistent Descriptor Allocator
		constexpr uint32_t kPersistentCap = 32768;
		perDescAlloc_.Init(&descHeap_, kPersistentCap);

		// ---- 配分 ----
		constexpr uint32_t kPersistentBase = 0;
		constexpr uint32_t kPersistentCap = 32768;
		constexpr uint32_t kTableBase = kPersistentBase + kPersistentCap;
		constexpr uint32_t kTableCapPerFrame = 8192;

		// =================================================
		// PerFrameResource 初期化 + descriptor 配線
		// =================================================
		frameResources_.resize(bufferCount_);
		for (uint32_t i = 0; i < bufferCount_; ++i) {
			frameResources_[i] = std::make_unique<PerFrameResource>();
			Utils::Exception::DX_CALL(
				frameResources_[i]->Init(GetDevice(), 16 * 1024)
			);

			frameResources_[i]->InitDescriptors(
				GDescHeap_,
				bufferCount_,
				kTableBase,
				kTableCapPerFrame
			);
		}
	}
	catch (const Utils::Exception::DxException& e) {
		OutputDebugStringA(e.what());
		MessageBoxA(nullptr, e.what(), "Fatal DirectX Error", MB_OK | MB_ICONERROR);
		std::terminate();
	}
}

void DX12Manager::Finalize()
{
	// GPU完了待ち
	if (graphicsCtx_) graphicsCtx_->WaitForGpu();
	if (uploadCtx_)   uploadCtx_->WaitForGpu();

	// unique_ptr / vector の破棄に任せる（二重破棄防止）
	frameResources_.clear();

	frameSync_.reset();
	framebuffer_.reset();
	swapChain_.reset();
	uploadCtx_.reset();
	graphicsCtx_.reset();
	dx12Device_.reset();
}

HRESULT DX12Manager::BeginFrame()
{
	if (!graphicsCtx_ || !framebuffer_ || !swapChain_ || !frameSync_) {
		Utils::Logger::Error("DX12Manager::BeginFrame - subsystem missing\n");
		return E_POINTER;
	}

	// === GPUフレーム同期 ===
	frameSync_->BeginFrame();
	const uint32_t frameIndex = frameSync_->GetFrameIndex();

	// === per-frame bookkeeping ===
	if (frameIndex < frameResources_.size() && frameResources_[frameIndex]) {
		frameResources_[frameIndex]->BeginFrame(frameIndex);
	}

	// === コマンドリスト準備 ===
	HRESULT hr = graphicsCtx_->MoveToNextFrame();
	if (FAILED(hr)) return hr;

	return S_OK;
}

HRESULT DX12Manager::EndFrame()
{
	if (!graphicsCtx_ || !swapChain_ || !framebuffer_ || !frameSync_) {
		Utils::Logger::Error("DX12Manager::EndFrame - subsystem missing\n");
		return E_POINTER;
	}

	const UINT index = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = framebuffer_->GetBackBuffer(index);
	ID3D12GraphicsCommandList* list = graphicsCtx_->GetList();
	if (!list || !backBuffer) return E_FAIL;

	TransitionToPresent(index);

	HRESULT hr = graphicsCtx_->ExecuteAndSignal();
	if (FAILED(hr)) return hr;

	hr = swapChain_->Present(1, 0);
	if (FAILED(hr)) return hr;

	frameSync_->EndFrame();
	return S_OK;
}

void DX12Manager::BeginGBufferPass()
{
	auto* list = graphicsCtx_->GetList();
	if (!list || !framebuffer_) return;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[3] = {
		framebuffer_->GetGBufferRtv(0),
		framebuffer_->GetGBufferRtv(1),
		framebuffer_->GetGBufferRtv(2)
	};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = framebuffer_->GetGBufferDsv();

	list->OMSetRenderTargets(3, rtvs, FALSE, &dsv);

	auto vp = GetMainViewport();
	auto sc = GetMainScissor();
	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);
}

void DX12Manager::ClearGBuffer()
{
	auto* list = graphicsCtx_->GetList();
	if (!list || !framebuffer_) return;
	framebuffer_->ClearGBuffer(list);
}

void DX12Manager::BeginBackBufferPass()
{
	(void)graphicsCtx_;

	const UINT index = swapChain_ ? swapChain_->GetCurrentBackBufferIndex() : 0;
	PrepareBackBuffer(index);
	BindBackBuffer(index);
}

void DX12Manager::ClearBackBuffer()
{
	auto* list = graphicsCtx_->GetList();
	if (!list || !framebuffer_ || !swapChain_) return;

	const UINT index = swapChain_->GetCurrentBackBufferIndex();

	// デバッグ用
	static auto start = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();

	FLOAT color[4] = {
		0.2f + 0.3f * std::sinf(t),
		0.3f + 0.2f * std::cosf(t * 0.7f),
		0.4f, 1.0f
	};

	framebuffer_->ClearRenderTarget(list, index, color);
	framebuffer_->ClearDepthStencil(list);
}

D3D12_VIEWPORT DX12Manager::GetMainViewport() const
{
	D3D12_VIEWPORT vp{};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(framebuffer_ ? framebuffer_->GetWidth() : 0);
	vp.Height = static_cast<float>(framebuffer_ ? framebuffer_->GetHeight() : 0);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	return vp;
}

D3D12_RECT DX12Manager::GetMainScissor() const
{
	D3D12_RECT rc{};
	rc.left = 0;
	rc.top = 0;
	rc.right = framebuffer_ ? framebuffer_->GetWidth() : 0;
	rc.bottom = framebuffer_ ? framebuffer_->GetHeight() : 0;
	return rc;
}

void DX12Manager::PrepareBackBuffer(UINT index)
{
	if (!graphicsCtx_ || !framebuffer_ || !swapChain_) return;

	ID3D12Resource* backBuffer = framebuffer_->GetBackBuffer(index);
	if (!backBuffer) return;

	const D3D12_RESOURCE_STATES prev = framebuffer_->GetBackBufferState(index);
	if (prev == D3D12_RESOURCE_STATE_RENDER_TARGET) return;

	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = backBuffer;
	b.Transition.StateBefore = prev;
	b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	graphicsCtx_->GetList()->ResourceBarrier(1, &b);
	framebuffer_->SetBackBufferState(index, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void DX12Manager::BindBackBuffer(UINT index)
{
	if (!graphicsCtx_ || !framebuffer_) return;

	auto* list = graphicsCtx_->GetList();
	if (!list) return;

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = framebuffer_->GetRtvHandle(index);
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = framebuffer_->GetDsvHandle();
	list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	auto vp = GetMainViewport();
	auto sc = GetMainScissor();
	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);
}

void DX12Manager::TransitionToPresent(UINT index)
{
	if (!graphicsCtx_ || !framebuffer_ || !swapChain_) return;

	ID3D12Resource* backBuffer = framebuffer_->GetBackBuffer(index);
	if (!backBuffer) return;

	const D3D12_RESOURCE_STATES prev = framebuffer_->GetBackBufferState(index);
	if (prev == D3D12_RESOURCE_STATE_PRESENT) return;

	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = backBuffer;
	b.Transition.StateBefore = prev;
	b.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	graphicsCtx_->GetList()->ResourceBarrier(1, &b);
	framebuffer_->SetBackBufferState(index, D3D12_RESOURCE_STATE_PRESENT);
}
