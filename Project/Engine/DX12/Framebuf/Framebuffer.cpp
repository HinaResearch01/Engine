#include "Framebuffer.h"
#include "DX12/DX12Manager.h"
#include "Utils/Logger/Logger.h"
#include "Win/Win32Window.h"
#include <d3dx12.h>

using namespace Tsumi::DX12;
using namespace Microsoft::WRL;

Framebuffer::Framebuffer(DX12Manager* ptr) 
	: dx12Mgr_(ptr)
{
}

Tsumi::DX12::Framebuffer::~Framebuffer()
{
	Destroy();
}

HRESULT Framebuffer::Init()
{
    if (!dx12Mgr_) return E_POINTER;
    ID3D12Device* device = dx12Mgr_->GetDevice();
    IDXGISwapChain4* swapChain = dx12Mgr_->GetIDXGISwapChain4();
    if (!device || !swapChain) {
        Utils::Logger::Error("Framebuffer::Initialize - device or swapChain is null\n");
        return E_POINTER;
    }

	Win32::Win32Desc desc = Win32::Win32Window::GetInstance()->GetDesc();
	width_ = desc.windowWidth;
	height_ = desc.windowHeight;

	return CreateHeapsAndViews(width_, height_);
}

void Tsumi::DX12::Framebuffer::Destroy()
{
    ReleaseViews();
}

HRESULT Framebuffer::Resize(UINT width, UINT height)
{
    // スワップチェーンのリサイズ処理
    if (!dx12Mgr_) return E_POINTER;
    IDXGISwapChain4* swapChain = dx12Mgr_->GetIDXGISwapChain4();
    if (!swapChain) return E_POINTER;

    // 既存リソースを破棄
    ReleaseViews();

    // バッファ数を取得（最低2枚）
    UINT bufCount = dx12Mgr_->GetBufferCount();
    if (bufCount < 2) bufCount = 2;

    // スワップチェーンのバッファを再作成
    HRESULT hr = swapChain->ResizeBuffers(bufCount, width, height, backBufferFormat_, 0);
    if (FAILED(hr)) {
        Utils::Logger::Error(
			"Framebuffer::Resize - ResizeBuffers failed (hr=0x{:08X})\n", 
			static_cast<unsigned>(hr));
        return hr;
    }

    width_ = width;
    height_ = height;

    // 新しいサイズでRTV/DSVを作成し直す
    CreateHeapsAndViews(width, height);	hr = CreateHeapsAndViews(width, height);

    // リサイズ後、バックバッファのステートを PRESENT に設定しておく
    if (SUCCEEDED(hr)) {
        backBufferStates_.assign(backBuffers_.size(), D3D12_RESOURCE_STATE_PRESENT);
    }
    return hr;
}

D3D12_CPU_DESCRIPTOR_HANDLE Framebuffer::GetRtvHandle(UINT index) const
{
    // 指定インデックスのRTVハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    if (!rtvHeap_) return handle;
    UINT bufCount = static_cast<UINT>(backBuffers_.size());
    if (index >= bufCount) return handle;
    handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr = SIZE_T(handle.ptr) + SIZE_T(index) * SIZE_T(rtvDescriptorSize_);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Framebuffer::GetDsvHandle() const
{
    // DSVハンドルを取得（1つのみ）
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    if (!dsvHeap_) return handle;
    handle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    return handle;
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
    // 指定されたRTVを指定色でクリア
    if (!cmdList) return;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvHandle(rtvIndex);
    if (rtvHandle.ptr == 0) return;
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void Framebuffer::ClearDepthStencil(ID3D12GraphicsCommandList* cmdList, FLOAT depth, UINT8 stencil)
{
    // 深度・ステンシルバッファをクリア
    if (!cmdList) return;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvHandle();
    if (dsvHandle.ptr == 0) return;
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
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

HRESULT Framebuffer::CreateHeapsAndViews(UINT width, UINT height)
{
	auto device = dx12Mgr_->GetDevice();

	// =========================
	// GBuffer RTV Heap
	// =========================
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
	rtvDesc.NumDescriptors = GBUFFER_COUNT;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&gbufferRtvHeap_));
	gbufferRtvDescriptorSize_ =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// =========================
	// GBuffer SRV Heap（連続）
	// =========================
	D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
	srvDesc.NumDescriptors = GBUFFER_COUNT + 1; // RT + Depth
	srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&gbufferSrvHeap_));
	gbufferSrvDescriptorSize_ =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// =========================
	// GBuffer Resources
	// =========================
	gbufferRTs_.resize(GBUFFER_COUNT);

	for (UINT i = 0; i < GBUFFER_COUNT; ++i)
	{
		CD3DX12_RESOURCE_DESC desc =
			CD3DX12_RESOURCE_DESC::Tex2D(
			gbufferFormats_[i],
			width, height,
			1, 1,
			1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		D3D12_CLEAR_VALUE clear{};
		clear.Format = gbufferFormats_[i];

		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clear,
			IID_PPV_ARGS(&gbufferRTs_[i]));

		// RTV
		D3D12_CPU_DESCRIPTOR_HANDLE rtv =
			CD3DX12_CPU_DESCRIPTOR_HANDLE(
			gbufferRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			i, gbufferRtvDescriptorSize_);

		device->CreateRenderTargetView(
			gbufferRTs_[i].Get(), nullptr, rtv);

		// SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		srv.Format = gbufferFormats_[i];
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Texture2D.MipLevels = 1;

		D3D12_CPU_DESCRIPTOR_HANDLE srvCpu =
			CD3DX12_CPU_DESCRIPTOR_HANDLE(
			gbufferSrvHeap_->GetCPUDescriptorHandleForHeapStart(),
			i, gbufferSrvDescriptorSize_);

		device->CreateShaderResourceView(
			gbufferRTs_[i].Get(), &srv, srvCpu);
	}

	// =========================
	// GBuffer Depth
	// =========================
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

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClear,
		IID_PPV_ARGS(&gbufferDepth_)
	);

	// DSV
	D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
	dsvDesc.NumDescriptors = 1;
	dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&gbufferDsvHeap_));

	device->CreateDepthStencilView(
		gbufferDepth_.Get(), nullptr,
		gbufferDsvHeap_->GetCPUDescriptorHandleForHeapStart());

	// Depth SRV（最後のスロット）
	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrv{};
	depthSrv.Format = DXGI_FORMAT_R32_FLOAT;
	depthSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthSrv.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvCpu =
		CD3DX12_CPU_DESCRIPTOR_HANDLE(
		gbufferSrvHeap_->GetCPUDescriptorHandleForHeapStart(),
		GBUFFER_COUNT, gbufferSrvDescriptorSize_);

	device->CreateShaderResourceView(
		gbufferDepth_.Get(), &depthSrv, depthSrvCpu);

	return S_OK;
}

void Framebuffer::ReleaseViews()
{
    for (auto& b : backBuffers_) b.Reset();
    backBuffers_.clear();
    depthStencil_.Reset();
    rtvHeap_.Reset();
    dsvHeap_.Reset();
    rtvDescriptorSize_ = 0;
    dsvDescriptorSize_ = 0;
    backBufferStates_.clear();
}