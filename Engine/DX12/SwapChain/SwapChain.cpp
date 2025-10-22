#include "SwapChain.h"
#include "Core/App/Application.h"
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;

SwapChain::SwapChain(DX12Manager* ptr)
{
    dx12Mgr_ = ptr;
}

HRESULT SwapChain::Create()
{
    HWND hwnd = Win32::Win32Window::GetInstance()->GetHWND();
    Win32::Win32Desc winDesc = Win32::Win32Window::GetInstance()->GetDesc();

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = winDesc.windowWidth;
    desc.Height = winDesc.windowHeight;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    // DX12Manager 側の bufferCount を尊重する（最小 2）
    UINT bufCount = 2;
    if (dx12Mgr_) bufCount = dx12Mgr_->GetBufferCount();
    desc.BufferCount = bufCount;

    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = dx12Mgr_->GetFactory()->CreateSwapChainForHwnd(
        dx12Mgr_->GetCmdQueue(),
        hwnd,
        &desc,
        nullptr,
        nullptr,
        &swapChain1);

    if (FAILED(hr)) return hr;

    swapChain1.As(&swapChain_); // IDXGISwapChain4にアップキャスト
    desc_ = desc;

    return S_OK;
}

void Tsumi::DX12::SwapChain::Present(UINT syncInterval, UINT flags)
{
    if (swapChain_) {
        swapChain_->Present(syncInterval, flags);
    }
}
