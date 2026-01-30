#include "Framebuffer.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorHeap.h"
#include "DX12/Desc/PersistentDescAllocator.h"
#include "Utils/Logger/Logger.h"
#include "Win/Win32Window.h"
#include <algorithm>
#undef max

using namespace Tsumi::DX12;
using namespace Microsoft::WRL;

namespace {
inline bool HRFailed(HRESULT hr, const char* where)
{
	if (FAILED(hr)) {
		Tsumi::Utils::Logger::Error("{} (hr=0x{:08X})\n", where, static_cast<unsigned>(hr));
		return true;
	}
	return false;
}
}

Framebuffer::Framebuffer(DX12Manager* ptr)
	: dx12Mgr_(ptr)
{}

Framebuffer::~Framebuffer() { Destroy(); }

HRESULT Framebuffer::Init()
{
	if (!dx12Mgr_) return E_POINTER;

	auto* device = dx12Mgr_->GetDevice();
	if (!device) return E_POINTER;

	Win32::Win32Desc desc = Win32::Win32Window::GetInstance()->GetDesc();
	width_ = desc.windowWidth;
	height_ = desc.windowHeight;

	if (!gbufferSrvBase_.valid()) {
		auto* per = dx12Mgr_->GetPersistentDescAllocator();
		if (!per) return E_POINTER;
		gbufferSrvBase_ = per->Allocate(GBUFFER_COUNT + 1);
		if (!gbufferSrvBase_.valid()) return E_FAIL;
	}

	return CreateHeapsAndViews(width_, height_);
}

void Framebuffer::Destroy()
{
	ReleaseViews();

	if (dx12Mgr_ && gbufferSrvBase_.valid()) {
		if (auto* per = dx12Mgr_->GetPersistentDescAllocator()) {
			per->Free(gbufferSrvBase_, GBUFFER_COUNT + 1);
		}
		gbufferSrvBase_ = {};
	}
}

HRESULT Framebuffer::Resize(UINT width, UINT height)
{
	if (!dx12Mgr_) return E_POINTER;

	// GPU完了待ち
	dx12Mgr_->WaitForGpu();

	IDXGISwapChain4* swapChain = dx12Mgr_->GetIDXGISwapChain4();
	if (!swapChain) return E_POINTER;

	ReleaseViews();

	// バッファ数
	const UINT bufCount = std::max<UINT>(2, dx12Mgr_->GetBufferCount());

	HRESULT hr = swapChain->ResizeBuffers(bufCount, width, height, backBufferFormat_, 0);
	if (FAILED(hr)) return hr;

	width_ = width;
	height_ = height;

	hr = CreateHeapsAndViews(width_, height_);
	if (FAILED(hr)) return hr;

	backBufferStates_.assign(backBuffers_.size(), D3D12_RESOURCE_STATE_PRESENT);
	return S_OK;
}

D3D12_CPU_DESCRIPTOR_HANDLE Framebuffer::GetRtvHandle(UINT index) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle{};
	if (!rtvHeap_) return handle;
	if (index >= backBuffers_.size()) return handle;

	handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += SIZE_T(index) * SIZE_T(rtvDescriptorSize_);
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Framebuffer::GetDsvHandle() const
{
	if (!dsvHeap_) return {};
	return dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* Framebuffer::GetBackBuffer(UINT index) const
{
	if (index >= backBuffers_.size()) return nullptr;
	return backBuffers_[index].Get();
}

UINT Framebuffer::GetBufferCount() const
{
	return static_cast<UINT>(backBuffers_.size());
}

void Framebuffer::ClearRenderTarget(ID3D12GraphicsCommandList* cmdList, UINT rtvIndex, const FLOAT clearColor[4])
{
	if (!cmdList) return;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvHandle(rtvIndex);
	if (rtvHandle.ptr == 0) return;
	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void Framebuffer::ClearDepthStencil(ID3D12GraphicsCommandList* cmdList, FLOAT depth, UINT8 stencil)
{
	if (!cmdList) return;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvHandle();
	if (dsvHandle.ptr == 0) return;
	cmdList->ClearDepthStencilView(
		dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		depth, stencil,
		0, nullptr
	);
}

D3D12_RESOURCE_STATES Framebuffer::GetBackBufferState(UINT index) const
{
	if (index >= backBufferStates_.size()) return D3D12_RESOURCE_STATE_COMMON;
	return backBufferStates_[index];
}

void Framebuffer::SetBackBufferState(UINT index, D3D12_RESOURCE_STATES state)
{
	if (index >= backBufferStates_.size()) return;
	backBufferStates_[index] = state;
}

D3D12_CPU_DESCRIPTOR_HANDLE Framebuffer::GetGBufferRtv(UINT index) const
{
	if (!gbufferRtvHeap_) return {};
	if (index >= GBUFFER_COUNT) return {};
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		gbufferRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
		index, gbufferRtvDescriptorSize_);
}

D3D12_CPU_DESCRIPTOR_HANDLE Framebuffer::GetGBufferDsv() const
{
	if (!gbufferDsvHeap_) return {};
	return gbufferDsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

const FLOAT* Framebuffer::GetGBufferClearColor(GBufferType type)
{
	switch (type) {
		case GBufferType::Albedo: {
			static const FLOAT c[4] = { 0,0,0,1 };
			return c;
		}
		case GBufferType::Normal: {
			static const FLOAT c[4] = { 0,0,1,1 };
			return c;
		}
		case GBufferType::Material: {
			static const FLOAT c[4] = { 1,0,1,1 };
			return c;
		}
		default: {
			static const FLOAT c[4] = { 0,0,0,1 };
			return c;
		}
	}
}

void Framebuffer::ClearGBuffer(ID3D12GraphicsCommandList* cmdList) const
{
	if (!cmdList) return;

	// RT
	for (UINT i = 0; i < GBUFFER_COUNT; ++i) {
		auto rtv = GetGBufferRtv(i);
		if (rtv.ptr == 0) continue;
		cmdList->ClearRenderTargetView(rtv, GetGBufferClearColor((GBufferType)i), 0, nullptr);
	}

	// Depth
	auto dsv = GetGBufferDsv();
	if (dsv.ptr != 0) {
		cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

HRESULT Framebuffer::CreateHeapsAndViews(UINT width, UINT height)
{
	if (!dx12Mgr_) return E_POINTER;

	ID3D12Device* device = dx12Mgr_->GetDevice();
	auto* swapChain = dx12Mgr_->GetIDXGISwapChain4();
	if (!device || !swapChain) return E_POINTER;

	if (!gbufferSrvBase_.valid()) return E_FAIL;

	ReleaseViews();

	const UINT bufferCount = std::max<UINT>(2, dx12Mgr_->GetBufferCount());

	// =========================================================
	// SwapChain RTV Heap + RTVs
	// =========================================================
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
		rtvDesc.NumDescriptors = bufferCount;
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		HRESULT hr = device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap_));
		if (HRFailed(hr, "CreateHeapsAndViews - Create RTV heap")) return hr;

		rtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		backBuffers_.resize(bufferCount);
		backBufferStates_.resize(bufferCount, D3D12_RESOURCE_STATE_PRESENT);

		for (UINT i = 0; i < bufferCount; ++i)
		{
			hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
			if (HRFailed(hr, "CreateHeapsAndViews - swapChain->GetBuffer")) return hr;

			device->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, GetRtvHandle(i));
		}
	}

	// =========================================================
	// Main Depth (DSV heap + resource)
	// =========================================================
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
		dsvDesc.NumDescriptors = 1;
		dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		HRESULT hr = device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&dsvHeap_));
		if (HRFailed(hr, "CreateHeapsAndViews - Create DSV heap")) return hr;

		dsvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		CD3DX12_RESOURCE_DESC depthDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
			depthStencilFormat_,
			width, height,
			1, 1,
			1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		D3D12_CLEAR_VALUE depthClear{};
		depthClear.Format = depthStencilFormat_;
		depthClear.DepthStencil.Depth = 1.0f;
		depthClear.DepthStencil.Stencil = 0;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClear,
			IID_PPV_ARGS(&depthStencil_));
		if (HRFailed(hr, "CreateHeapsAndViews - Create main depth")) return hr;

		device->CreateDepthStencilView(depthStencil_.Get(), nullptr, GetDsvHandle());
	}

	// =========================================================
	// GBuffer RTV Heap
	// =========================================================
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.NumDescriptors = GBUFFER_COUNT;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gbufferRtvHeap_));
		if (HRFailed(hr, "CreateHeapsAndViews - Create GBuffer RTV heap")) return hr;

		gbufferRtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// =========================================================
	// GBuffer SRV (Persistent, GlobalDescriptorHeap 上)
	//   [0..2] RT SRV
	//   [3]    Depth SRV
	// =========================================================
	assert(gbufferSrvBase_.valid());

	// Global heap の increment size（CBV/SRV/UAV）
	const UINT inc = dx12Mgr_->GetGlobalDescriptorStride();

	// =========================================================
	// GBuffer RT Resources + RTV/SRV
	// =========================================================
	gbufferRTs_.resize(GBUFFER_COUNT);

	for (UINT i = 0; i < GBUFFER_COUNT; ++i)
	{
		CD3DX12_RESOURCE_DESC rtDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
			gbufferFormats_[i],
			width, height,
			1, 1,
			1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		D3D12_CLEAR_VALUE clear{};
		clear.Format = gbufferFormats_[i];

		const FLOAT* c = Framebuffer::GetGBufferClearColor(static_cast<GBufferType>(i));
		clear.Color[0] = c[0];
		clear.Color[1] = c[1];
		clear.Color[2] = c[2];
		clear.Color[3] = c[3];

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&rtDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clear,
			IID_PPV_ARGS(&gbufferRTs_[i]));
		if (HRFailed(hr, "CreateHeapsAndViews - Create GBuffer RT")) return hr;

		device->CreateRenderTargetView(gbufferRTs_[i].Get(), nullptr, GetGBufferRtv(i));

		// SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		srv.Format = gbufferFormats_[i];
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Texture2D.MipLevels = 1;

		D3D12_CPU_DESCRIPTOR_HANDLE srvCpu = gbufferSrvBase_.cpu;
		srvCpu.ptr += SIZE_T(i) * SIZE_T(inc);

		device->CreateShaderResourceView(gbufferRTs_[i].Get(), &srv, srvCpu);
	}

	// =========================================================
	// GBuffer Depth (resource + DSV + Depth SRV)
	// =========================================================
	{
		CD3DX12_RESOURCE_DESC depthDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT,
			width, height,
			1, 1,
			1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		D3D12_CLEAR_VALUE depthClear{};
		depthClear.Format = DXGI_FORMAT_D32_FLOAT;
		depthClear.DepthStencil.Depth = 1.0f;
		depthClear.DepthStencil.Stencil = 0;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClear,
			IID_PPV_ARGS(&gbufferDepth_));
		if (HRFailed(hr, "CreateHeapsAndViews - Create GBuffer Depth")) return hr;

		D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
		dsvDesc.NumDescriptors = 1;
		dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&gbufferDsvHeap_));
		if (HRFailed(hr, "CreateHeapsAndViews - Create GBuffer DSV heap")) return hr;

		device->CreateDepthStencilView(gbufferDepth_.Get(), nullptr, gbufferDsvHeap_->GetCPUDescriptorHandleForHeapStart());

		// Depth SRV（D32_FLOAT は R32_FLOAT で読む）
		D3D12_SHADER_RESOURCE_VIEW_DESC depthSrv{};
		depthSrv.Format = DXGI_FORMAT_R32_FLOAT;
		depthSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		depthSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		depthSrv.Texture2D.MipLevels = 1;

		D3D12_CPU_DESCRIPTOR_HANDLE depthSrvCpu = gbufferSrvBase_.cpu;
		depthSrvCpu.ptr += SIZE_T(GBUFFER_COUNT) * SIZE_T(inc);

		device->CreateShaderResourceView(gbufferDepth_.Get(), &depthSrv, depthSrvCpu);
	}

	return S_OK;
}

void Framebuffer::ReleaseViews()
{
	// SwapChain
	for (auto& b : backBuffers_) b.Reset();
	backBuffers_.clear();
	backBufferStates_.clear();

	// Main depth
	depthStencil_.Reset();

	rtvHeap_.Reset();
	dsvHeap_.Reset();
	rtvDescriptorSize_ = 0;
	dsvDescriptorSize_ = 0;

	// GBuffer
	for (auto& rt : gbufferRTs_) rt.Reset();
	gbufferRTs_.clear();

	gbufferDepth_.Reset();

	gbufferRtvHeap_.Reset();
	gbufferDsvHeap_.Reset();

	gbufferRtvDescriptorSize_ = 0;
}
