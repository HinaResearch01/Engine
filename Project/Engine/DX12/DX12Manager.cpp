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
	resourceCtx_ = std::make_unique<CommandContext>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	device_ = std::make_unique<DX12Device>();
	swapChain_ = std::make_unique<SwapChain>(this);
	framebuffer_ = std::make_unique<Framebuffer>(this);
	frameSync_ = std::make_unique<FrameSync>(this);
}

void DX12Manager::Init()
{
	try {
		// ---- device ----
		Utils::Exception::DX_CALL(device_->Create());

		// ---- contexts ----
		Utils::Exception::DX_CALL(graphicsCtx_->Create(bufferCount_));
		Utils::Exception::DX_CALL(uploadCtx_->Create(bufferCount_));
		Utils::Exception::DX_CALL(resourceCtx_->Create(1));

		// ---- swapchain first ----
		Utils::Exception::DX_CALL(swapChain_->Create(desiredBufferCount_));
		bufferCount_ = swapChain_->GetBufferCount();
		Utils::Logger::Info("SwapChain BufferCount = {}\n", bufferCount_);

		// ---- descriptors + frames ----
		InitDescriptors();
		InitFrames();

		// ---- framebuffer / sync ----
		Utils::Exception::DX_CALL(framebuffer_->Init());
		Utils::Exception::DX_CALL(frameSync_->Init());
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

	frames_.clear();

	perDescAlloc_.Shutdown();
	descHeap_.Finalize();

	framebuffer_.reset();
	swapChain_.reset();
	
	uploadCtx_.reset();
	graphicsCtx_.reset();
	resourceCtx_.reset();

	frameSync_.reset();
	device_.reset();
}

HRESULT DX12Manager::BeginFrame()
{
	if (!graphicsCtx_ || !swapChain_ || !framebuffer_ || !frameSync_) {
		Utils::Logger::Error("DX12Manager::BeginFrame - subsystem missing\n");
		return E_POINTER;
	}

	// ---- CPU frame sync ----
	cpuFrameIndex_ = (cpuFrameIndex_ + 1) % bufferCount_;
	frameSync_->BeginFrame(cpuFrameIndex_);

	// ---- Reset command list ----
	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
	HRESULT hr = graphicsCtx_->ResetForFrame(cpuFrameIndex_);
	if (FAILED(hr)) return hr;

	// ---- descriptor deferred 回収 ----
	perDescAlloc_.ReleaseDeferred(cpuFrameIndex_);

	// ---- per-frame alloc reset ----
	frames_[cpuFrameIndex_].Begin(*graphicsCtx_);

	// ---- global heap bind ----
	ID3D12DescriptorHeap* heaps[] = { descHeap_.GetHeap() };
	graphicsCtx_->SetDescriptorHeaps(1, heaps);

	return S_OK;
}

HRESULT DX12Manager::EndFrame()
{
	if (!graphicsCtx_ || !swapChain_ || !framebuffer_ || !frameSync_) {
		Utils::Logger::Error("DX12Manager::EndFrame - subsystem missing\n");
		return E_POINTER;
	}

	// ---- backbuffer → PRESENT ----
	TransitionToPresent(frameIndex_);

	// ---- execute ----
	HRESULT hr = graphicsCtx_->Execute();
	if (FAILED(hr)) return hr;

	// ---- present ----
	hr = swapChain_->Present(1, 0);
	if (FAILED(hr)) return hr;

	// ---- frame sync ----
	frameSync_->EndFrame(cpuFrameIndex_);

	return S_OK;
}

void DX12Manager::BeginGBufferPass()
{
	auto* list = graphicsCtx_ ? graphicsCtx_->GetList() : nullptr;
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
	auto* list = graphicsCtx_ ? graphicsCtx_->GetList() : nullptr;
	if (!list || !framebuffer_) return;
	framebuffer_->ClearGBuffer(list);
}

void DX12Manager::BeginBackBufferPass()
{
	if (!graphicsCtx_ || !framebuffer_ || !swapChain_) return;

	// ---- RT へ遷移 ----
	PrepareBackBuffer(frameIndex_);

	// ---- バインド ----
	BindBackBuffer(frameIndex_);
}

void DX12Manager::ClearBackBuffer()
{
	auto* list = graphicsCtx_ ? graphicsCtx_->GetList() : nullptr;
	if (!list || !framebuffer_ || !swapChain_) return;

	static auto start = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();

	FLOAT color[4] = {
		0.2f + 0.3f * std::sinf(t),
		0.3f + 0.2f * std::cosf(t * 0.7f),
		0.4f, 1.0f
	};

	framebuffer_->ClearRenderTarget(list, frameIndex_, color);
	framebuffer_->ClearDepthStencil(list);
}

void DX12Manager::WaitForGpu()
{
	if (graphicsCtx_) {
		graphicsCtx_->WaitForGpu(); 
	}
	if (uploadCtx_) {
		uploadCtx_->WaitForGpu();
	}
}

D3D12_VIEWPORT DX12Manager::GetMainViewport() const
{
	float w = static_cast<float>(framebuffer_ ? framebuffer_->GetWidth() : 0);
	float h = static_cast<float>(framebuffer_ ? framebuffer_->GetHeight() : 0);

	if (w <= 0.0f || h <= 0.0f) {
		Tsumi::Utils::Logger::Warn("Viewport Size is INVALID! ({}, {})\n", w, h);
	}

	D3D12_VIEWPORT vp{};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = w;
	vp.Height = h;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	return vp;
}

D3D12_RECT DX12Manager::GetMainScissor() const
{
	D3D12_RECT rc{};
	rc.left = 0;
	rc.top = 0;
	rc.right = framebuffer_ ? (LONG)framebuffer_->GetWidth() : 0;
	rc.bottom = framebuffer_ ? (LONG)framebuffer_->GetHeight() : 0;
	return rc;
}

void DX12Manager::TransitionGBufferToWrite()
{
	auto* list = graphicsCtx_ ? graphicsCtx_->GetList() : nullptr;
	if (!list || !framebuffer_) return;
	framebuffer_->TransitionGBufferToWrite(list);
}

void DX12Manager::TransitionGBufferToRead()
{
	auto* list = graphicsCtx_ ? graphicsCtx_->GetList() : nullptr;
	if (!list || !framebuffer_) return;
	framebuffer_->TransitionGBufferToRead(list);
}

void DX12Manager::InitDescriptors()
{
	// 物理ヒープ：CBV/SRV/UAV
	descHeap_.Init(GetDevice(), totalDescriptors_, true);
	// 永続 allocator
	perDescAlloc_.Init(&descHeap_, 0, persistentCap_, bufferCount_);
}

void DX12Manager::InitFrames()
{
	assert(bufferCount_ >= 2);
	frames_.resize(bufferCount_);

	const uint32_t transientBase = persistentCap_;
	for (uint32_t i = 0; i < bufferCount_; ++i) {
		frames_[i].Init(
			GetDevice(), 
			uploadBytesPerFrame_, 
			&descHeap_,
			transientBase + i * transientCapPerFrame_,
			transientCapPerFrame_);
		frames_[i].fenceValue = 0;
	}
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
