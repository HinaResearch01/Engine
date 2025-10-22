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

    // 画面のクライアント領域
    desc_.Width = winDesc.windowWidth;
    desc_.Height = winDesc.windowHeight;
    // 色の形式
    desc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // マルチサンプルしない
    desc_.SampleDesc.Count = 1;
    // 描画のターゲットとして利用する
    desc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
    // ダブルバッファ
    desc_.BufferCount = 2;
    // モニタにうつしたら、中身を破棄
    desc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 


    // コマンドキューに設定を渡して生成
    HRESULT hr = S_OK;
    hr = dx12Mgr_->GetFactory()->CreateSwapChainForHwnd(
        dx12Mgr_->GetCmdQueue(),
        hwnd, &desc_, nullptr, nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

void Tsumi::DX12::SwapChain::Present(UINT syncInterval, UINT flags)
{
    if (swapChain_) {
        swapChain_->Present(syncInterval, flags);
    }
}
