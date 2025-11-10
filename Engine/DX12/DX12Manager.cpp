#include "DX12Manager.h"
#include "Utils/Logger/UtilsLog.h"
#include <chrono>
#include <format>

using namespace Tsumi::DX12;

DX12Manager::DX12Manager()
{
	cmdContext_ = std::make_unique<CommandContext>(this);
	descAlloc_ = std::make_unique<DescriptorAllocator>(this);
	dx12Device_ = std::make_unique<DX12Device>();
	swapChain_ = std::make_unique<SwapChain>(this);
	framebuf_ = std::make_unique<Framebuffer>(this);
	frameSync_ = std::make_unique<FrameSync>(this);
}

void DX12Manager::Init()
{
	try {
		Utils::DX_CALL(dx12Device_->Create());
		if(cmdContext_) cmdContext_->SetFrameCount(bufferCount_);
		Utils::DX_CALL(cmdContext_->Create());
		Utils::DX_CALL(swapChain_->Create());
		Utils::DX_CALL(framebuf_->Init());
		Utils::DX_CALL(frameSync_->Init());
		Utils::DX_CALL(descAlloc_->Init());
	}
	catch (const Utils::DxException& e) {
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
	if(descAlloc_) descAlloc_.reset();
}

HRESULT DX12Manager::StartFrame()
{
	if (!cmdContext_ || !framebuf_ || !swapChain_ || !frameSync_) {
		Utils::Log(L"DX12Manager::StartFrame - subsystem missing\n");
		return E_POINTER;
	}

	// === GPUフレーム同期 ===
	frameSync_->BeginFrame(); // GPUがこのフレームの使用を終えるまで待機
	//const uint32_t frameIndex = frameSync_->GetFrameIndex();

	if (descAlloc_) {
		descAlloc_->Reset();
	}

	// === コマンドリストの準備 ===
	HRESULT hr = cmdContext_->MoveToNextFrame();
	if (FAILED(hr)) return hr;
	ID3D12GraphicsCommandList* list = cmdContext_->GetList();
	if (!list) return E_FAIL;

	// === バックバッファの準備 ===
	UINT currIndex = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = framebuf_->GetBackBuffer(currIndex);
	if (!backBuffer) return E_FAIL;

	PrepareBackBuffer(currIndex, backBuffer);
	BindRenderTargets(currIndex);
	ClearRenderTargets(currIndex);

	return S_OK;
}

HRESULT DX12Manager::EndFrame()
{
	if (!cmdContext_ || !swapChain_ || !framebuf_ || !frameSync_) {
		Utils::Log(L"DX12Manager::EndFrame - subsystem missing\n");
		return E_POINTER;
	}

	UINT currIndex = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = framebuf_->GetBackBuffer(currIndex);
	ID3D12GraphicsCommandList* list = cmdContext_->GetList();
	if (!list || !backBuffer) return E_FAIL;

	// === RenderTarget -> Present へ遷移 ===
	TransitionToPresent(currIndex, backBuffer);

	// === コマンド送信と Present ===
	HRESULT hr = cmdContext_->ExecuteAndSignal();
	if (FAILED(hr)) return hr;

	hr = swapChain_->Present(1, 0);
	if (FAILED(hr)) return hr;

	// === GPUにSignalして次フレームへ ===
	frameSync_->EndFrame();

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

void DX12Manager::PrepareBackBuffer(UINT currIndex, ID3D12Resource* backBuffer)
{
	D3D12_RESOURCE_STATES prevState = framebuf_->GetBackBufferState(currIndex);
	if (prevState == D3D12_RESOURCE_STATE_RENDER_TARGET) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.StateBefore = prevState;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmdContext_->GetList()->ResourceBarrier(1, &barrier);
	framebuf_->SetBackBufferState(currIndex, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void DX12Manager::BindRenderTargets(UINT currIndex)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = framebuf_->GetRtvHandle(currIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = framebuf_->GetDsvHandle();
	cmdContext_->GetList()->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void DX12Manager::ClearRenderTargets(UINT currIndex)
{
	static auto start = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();
	FLOAT color[4] = {
		0.2f + 0.3f * std::sinf(t),
		0.3f + 0.2f * std::cosf(t * 0.7f),
		0.4f, 1.0f
	};
	framebuf_->ClearRenderTarget(cmdContext_->GetList(), currIndex, color);
	framebuf_->ClearDepthStencil(cmdContext_->GetList());
}

void DX12Manager::TransitionToPresent(UINT currIndex, ID3D12Resource* backBuffer)
{
	D3D12_RESOURCE_STATES prevState = framebuf_->GetBackBufferState(currIndex);
	if (prevState == D3D12_RESOURCE_STATE_PRESENT) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.StateBefore = prevState;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmdContext_->GetList()->ResourceBarrier(1, &barrier);
	framebuf_->SetBackBufferState(currIndex, D3D12_RESOURCE_STATE_PRESENT);
}
