#include "DX12Manager.h"
#include "Utils/Logger/UtilsLog.h"
#include <chrono>

using namespace Tsumi::DX12;

DX12Manager::DX12Manager()
{
	dx12Device_ = std::make_unique<DX12Device>();
	cmdContext_ = std::make_unique<CommandContext>(this);
	swapChain_ = std::make_unique<SwapChain> (this);
	framebuf_ = std::make_unique<Framebuffer>(this);
}

void DX12Manager::Init()
{
	try {
		DX_CALL(dx12Device_->Create());
		if(cmdContext_) cmdContext_->SetFrameCount(bufferCount_);
		DX_CALL(cmdContext_->Create());
		DX_CALL(swapChain_->Create());
		DX_CALL(framebuf_->Init());
	}
	catch (const DxException& e) {
		// Visual Studio の出力ウィンドウにメッセージを出す
		OutputDebugStringA(e.what());

		// ユーザーに通知して終了
		MessageBoxA(nullptr, e.what(), "Fatal DirectX Error", MB_OK | MB_ICONERROR);
		std::terminate();
	}
}

HRESULT DX12Manager::StartFrame()
{
	const auto t0 = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float>(now - t0).count();
	FLOAT clearColor[4] = { 0.2f + 0.3f * std::sinf(t), 0.3f + 0.2f * std::cosf(t * 0.7f), 0.4f, 1.0f };

	if (!cmdContext_ || !framebuf_ || !swapChain_) {
		Utils::Log(L"DX12Manager::StartFrame - required subsystem missing\n");
		return E_POINTER;
	}

	// Prepare allocator / command list for this frame
	HRESULT hr = cmdContext_->MoveToNextFrame();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"DX12Manager::StartFrame - MoveToNextFrame failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	// Get command list in recording state
	ID3D12GraphicsCommandList* list = cmdContext_->GetList();
	if (!list) {
		Utils::Log(L"DX12Manager::StartFrame - command list is null\n");
		return E_FAIL;
	}

	// Clear current backbuffer and depth using Framebuffer helper
	UINT currIndex = swapChain_->GetCurrentBackBufferIndex();
	if (!clearColor) {
		FLOAT defaultClear[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
		framebuf_->ClearRenderTarget(list, currIndex, defaultClear);
	}
	else {
		framebuf_->ClearRenderTarget(list, currIndex, clearColor);
	}
	framebuf_->ClearDepthStencil(list);

	return S_OK;
}

HRESULT DX12Manager::EndFrame()
{
	if (!cmdContext_ || !swapChain_) {
		Utils::Log(L"DX12Manager::EndFrame - required subsystem missing\n");
		return E_POINTER;
	}

	// Submit command list and signal fence
	HRESULT hr = cmdContext_->ExecuteAndSignal();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"DX12Manager::EndFrame - ExecuteAndSignal failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	// Present
	hr = swapChain_->Present(1, 0);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"DX12Manager::EndFrame - Present failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	return S_OK;
}

void DX12Manager::PreDraw4PE()
{
}

void DX12Manager::PostDraw4PE()
{	 
}	 
	 
void DX12Manager::PreDraw4SC()
{	 
}	 
	 
void DX12Manager::PostDraw4SC()
{
}

void DX12Manager::OnFinalize()
{
}