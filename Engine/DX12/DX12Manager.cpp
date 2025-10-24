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

void DX12Manager::OnFinalize()
{
	if (cmdContext_) cmdContext_->WaitForGpu();
}

HRESULT DX12Manager::StartFrame()
{
	// compute a simple animated clear color (kept for backward compatibility)
	static auto start_time = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float>(now - start_time).count();
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

	// Transition back buffer (PRESENT/COMMON -> RENDER_TARGET)
	UINT currIndex = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = framebuf_->GetBackBuffer(currIndex);
	if (!backBuffer) {
		Utils::Log(L"DX12Manager::StartFrame - backBuffer is null\n");
		return E_FAIL;
	}

	D3D12_RESOURCE_BARRIER barrierToRT{};
	barrierToRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierToRT.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierToRT.Transition.pResource = backBuffer;
	// Most commonly the swapchain resource is in PRESENT state; if it's in COMMON this transition will still be valid for typical usage.
	barrierToRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierToRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierToRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	list->ResourceBarrier(1, &barrierToRT);

	// Bind RTV/DSV
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = framebuf_->GetRtvHandle(currIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = framebuf_->GetDsvHandle();
	list->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear current backbuffer and depth using Framebuffer helper
	framebuf_->ClearRenderTarget(list, currIndex, clearColor);
	framebuf_->ClearDepthStencil(list);

	return S_OK;
}

HRESULT DX12Manager::EndFrame()
{
	if (!cmdContext_ || !swapChain_ || !framebuf_) {
		Utils::Log(L"DX12Manager::EndFrame - required subsystem missing\n");
		return E_POINTER;
	}

	// Transition back buffer (RENDER_TARGET -> PRESENT)
	UINT currIndex = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = framebuf_->GetBackBuffer(currIndex);
	if (!backBuffer) {
		Utils::Log(L"DX12Manager::EndFrame - backBuffer is null\n");
		return E_FAIL;
	}

	ID3D12GraphicsCommandList* list = cmdContext_->GetList();
	if (!list) {
		Utils::Log(L"DX12Manager::EndFrame - command list is null\n");
		return E_FAIL;
	}

	D3D12_RESOURCE_BARRIER barrierToPresent{};
	barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierToPresent.Transition.pResource = backBuffer;
	barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrierToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	list->ResourceBarrier(1, &barrierToPresent);

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
