#include "SwapChain.h"
#include "Core/App/Application.h"
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"
#include "Utils/Logger/Logger.h"

using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;

SwapChain::SwapChain(DX12Manager* ptr)
    : dx12Mgr_(ptr)
{
}

HRESULT SwapChain::Create(UINT desiredBufferCount)
{
	if (!dx12Mgr_) return E_POINTER;

	HWND hwnd = Win32::Win32Window::GetInstance()->GetHWND();
	auto winDesc = Win32::Win32Window::GetInstance()->GetDesc();

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Width = winDesc.windowWidth;
	desc.Height = winDesc.windowHeight;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc = { 1, 0 };
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	UINT count = desiredBufferCount;
	if (count < 2) count = 2;
	if (count > 3) count = 3; 
	desc.BufferCount = count;

	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.Flags = 0;

	IDXGIFactory4* factory4 = dx12Mgr_->GetFactory();
	if (!factory4) return E_POINTER;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> sc1;
	HRESULT hr = factory4->CreateSwapChainForHwnd(
		dx12Mgr_->GetGraphicsQueue(), hwnd, &desc, nullptr, nullptr, &sc1);
	if (FAILED(hr)) return hr;

	hr = sc1.As(&swapChain_);
	if (FAILED(hr) || !swapChain_) return FAILED(hr) ? hr : E_FAIL;

	desc_ = desc;
	factory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	return S_OK;
}

HRESULT Tsumi::DX12::SwapChain::Present(UINT syncInterval, UINT flags)
{
    if (!swapChain_) return E_POINTER;

    HRESULT hr = swapChain_->Present(syncInterval, flags);
    if (FAILED(hr)) {
        Utils::Logger::Error(
			"SwapChain::Present failed",
			"hr", static_cast<unsigned>(hr));
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
        Utils::Logger::Error(
			"SwapChain::GetBuffer failed for index", 
			"index", static_cast<UINT>(index), 
			"hr", static_cast<unsigned>(hr));
    }
    return hr;
}

HRESULT SwapChain::Resize(UINT width, UINT height)
{
	if (!swapChain_) return E_POINTER;

	for (auto& bb : backBuffers_) bb.Reset();

	const UINT count = desc_.BufferCount;
	HRESULT hr = swapChain_->ResizeBuffers(count, width, height, desc_.Format, desc_.Flags);
	if (FAILED(hr)) return hr;

	desc_.Width = width;
	desc_.Height = height;
	return S_OK;
}
