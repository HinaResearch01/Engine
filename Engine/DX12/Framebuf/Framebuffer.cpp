#include "Framebuffer.h"
#include "DX12/DX12Manager.h"
#include "Utils/Logger/UtilsLog.h"
#include "Win/Win32Window.h"

using namespace Tsumi::DX12;

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
    IDXGISwapChain4* swapChain = dx12Mgr_->GetSwapChain();
    if (!device || !swapChain) {
        Utils::Log(L"Framebuffer::Initialize - device or swapChain is null\n");
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
    IDXGISwapChain4* swapChain = dx12Mgr_->GetSwapChain();
    if (!swapChain) return E_POINTER;

    // 既存リソースを破棄
    ReleaseViews();

    // バッファ数を取得（最低2枚）
    UINT bufCount = dx12Mgr_->GetBufferCount();
    if (bufCount < 2) bufCount = 2;

    // スワップチェーンのバッファを再作成
    HRESULT hr = swapChain->ResizeBuffers(bufCount, width, height, backBufferFormat_, 0);
    if (FAILED(hr)) {
        Utils::Log(std::format(L"Framebuffer::Resize - ResizeBuffers failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }

    width_ = width;
    height_ = height;

    // 新しいサイズでRTV/DSVを作成し直す
    return CreateHeapsAndViews(width, height);
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
    if (!dx12Mgr_) return E_POINTER;
    ID3D12Device* device = dx12Mgr_->GetDevice();
    IDXGISwapChain4* swapChain = dx12Mgr_->GetSwapChain();
    if (!device || !swapChain) return E_POINTER;

    // バッファ枚数を取得（最低2）
    UINT bufferCount = dx12Mgr_->GetBufferCount();
    if (bufferCount < 2) bufferCount = 2;

    // 既存リソースを開放
    ReleaseViews();

    // === RTVヒープの作成 ===
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = bufferCount;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap_));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"Framebuffer::CreateHeapsAndViews - CreateDescriptorHeap(RTV) failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }
    rtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // === DSVヒープの作成（1つだけ） ===
    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&dsvHeap_));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"Framebuffer::CreateHeapsAndViews - CreateDescriptorHeap(DSV) failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }
    dsvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
   
    // === バックバッファのRTV作成 ===
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    backBuffers_.resize(bufferCount);
    for (UINT i = 0; i < bufferCount; ++i) {
        ComPtr<ID3D12Resource> backBuffer;
        // スワップチェーンから各バッファを取得
        hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr)) {
            Utils::Log(std::format(L"Framebuffer::CreateHeapsAndViews - GetBuffer({}) failed (hr=0x{:08X})\n", i, static_cast<unsigned>(hr)));
            return hr;
        }
        backBuffers_[i] = backBuffer;
        // RTVを作成
        device->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);
        // ハンドルを次の位置へ進める
        rtvHandle.ptr = SIZE_T(rtvHandle.ptr) + SIZE_T(rtvDescriptorSize_);
    }

    // === 深度ステンシルバッファの作成 ===
    D3D12_RESOURCE_DESC depthDesc{};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = depthStencilFormat_;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // GPU上に確保するヒープ設定（通常のDEFAULTヒープ）
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // 初期クリア値（深度=1.0, ステンシル=0）
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = depthStencilFormat_;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // 深度ステンシル用テクスチャを生成
    hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&depthStencil_)
    );

    if (FAILED(hr)) {
        Utils::Log(std::format(L"Framebuffer::CreateHeapsAndViews - CreateCommittedResource(depth) failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }

    // DSVを作成
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    device->CreateDepthStencilView(depthStencil_.Get(), nullptr, dsvHandle);

    backBufferStates_.resize(bufferCount, D3D12_RESOURCE_STATE_PRESENT);

    Utils::Log(std::format(L"Framebuffer initialized: {}x{}, buffers={}\n", width, height, bufferCount));
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