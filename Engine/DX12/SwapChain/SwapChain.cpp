#include "SwapChain.h"
#include "Core/App/Application.h"
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"
#include "Utils/Logger/UtilsLog.h"

using namespace Tsumi::DX12;

SwapChain::SwapChain(DX12Manager* ptr)
{
    dx12Mgr_ = ptr;
}

HRESULT SwapChain::Create()
{
    if (!dx12Mgr_) return E_POINTER;

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
    if (bufCount < 2) bufCount = 2;
    desc.BufferCount = bufCount;

    // もし「ティアリング（画面のズレ）」を許可したい場合は
    // DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING フラグを追加することを検討。
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // ウィンドウリサイズ時のスケーリング設定
    // DXGI_SCALING_STRETCH：ウィンドウサイズに合わせて引き伸ばす
    // DXGI_SCALING_NONE：ピクセル等倍で表示する
    desc.Scaling = DXGI_SCALING_STRETCH;

    // アルファ値（透明度）の扱い設定
    // 通常のウィンドウでは透明処理を行わないため IGNORE でOK
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    desc.Flags = 0;

    // DXGIファクトリを取得して SwapChain を作成する準備
    IDXGIFactory4* factory4 = dx12Mgr_->GetFactory();
    if (!factory4) {
        Utils::Log(L"Error: IDXGIFactory4 is null in SwapChain::Create()\n");
        return E_POINTER;
    }

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = factory4->CreateSwapChainForHwnd(
        dx12Mgr_->GetCmdQueue(), // コマンドキューを関連付ける
        hwnd,                    // 対象のウィンドウハンドル
        &desc,                   // スワップチェーンの設定
        nullptr,                 // フルスクリーン用の設定（今回は未使用）
        nullptr,                 // 出力先モニタ指定（未使用）
        &swapChain1);            // 作成されたスワップチェーンを受け取る

    if (FAILED(hr)) {
        Utils::Log(std::format(L"Error: CreateSwapChainForHwnd failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }

    // IDXGISwapChain1 → IDXGISwapChain4 にキャスト
    hr = swapChain1.As(&swapChain_);
    if (FAILED(hr) || !swapChain_) {
        Utils::Log(std::format(L"Error: swapChain1.As -> IDXGISwapChain4 failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }

    desc_ = desc;

    // Alt+Enterによる全画面切り替えを無効化
    factory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    return S_OK;
}

HRESULT Tsumi::DX12::SwapChain::Present(UINT syncInterval, UINT flags)
{
    if (!swapChain_) return E_POINTER;

    HRESULT hr = swapChain_->Present(syncInterval, flags);
    if (FAILED(hr)) {
        Utils::Log(std::format(L"SwapChain::Present failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
    }
    return hr;
}

UINT Tsumi::DX12::SwapChain::GetCurrentBackBufferIndex() const
{
    if (!swapChain_) return 0;
    return swapChain_->GetCurrentBackBufferIndex();
}

HRESULT SwapChain::GetBuffer(UINT index, ID3D12Resource** outResource) const
{
    if (!swapChain_) return E_POINTER;
    if (!outResource) return E_POINTER;
    if (index >= desc_.BufferCount) return E_INVALIDARG;

    HRESULT hr = swapChain_->GetBuffer(index, IID_PPV_ARGS(outResource));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"SwapChain::GetBuffer failed for index {} (hr=0x{:08X})\n", index, static_cast<unsigned>(hr)));
    }
    return hr;
}

HRESULT SwapChain::Resize(UINT width, UINT height)
{
    if (!swapChain_) return E_POINTER;

    for (UINT i = 0; i < _countof(backBuffers_); ++i) backBuffers_[i].Reset();

    // Use DX12Manager's authoritative buffer count via dx12Mgr_->GetBufferCount()
    UINT bufCount = 2;
    if (dx12Mgr_) bufCount = dx12Mgr_->GetBufferCount();
    if (bufCount < 2) bufCount = 2;

    HRESULT hr = swapChain_->ResizeBuffers(bufCount, width, height, desc_.Format, 0);
    if (FAILED(hr)) {
        Utils::Log(std::format(L"SwapChain::Resize - ResizeBuffers failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }

    desc_.Width = width;
    desc_.Height = height;
    Utils::Log(std::format(L"SwapChain resized: {}x{}\n", width, height));
    return S_OK;
}
