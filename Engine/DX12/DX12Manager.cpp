#include "DX12Manager.h"
#include "Utils/Logger/UtilsLog.h"
#include <chrono>

using namespace Tsumi::DX12;

DX12Manager::DX12Manager()
{
	dx12Device_ = std::make_unique<DX12Device>();
	cmdContext_ = std::make_unique<CommandContext>(this);
	swapChain_ = std::make_unique<SwapChain>(this);
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
	// --- 動的クリア色（お試し用） ---
	static auto start_time = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float>(now - start_time).count();
	FLOAT clearColor[4] = {
		0.2f + 0.3f * std::sinf(t),
		0.3f + 0.2f * std::cosf(t * 0.7f),
		0.4f,
		1.0f
	};

	// --- 必須サブシステムチェック ---
	if (!cmdContext_ || !framebuf_ || !swapChain_) {
		Utils::Log(L"DX12Manager::StartFrame - 必要なサブシステムが存在しません\n");
		return E_POINTER;
	}

	// --- 次フレームの準備（アロケータ／リストリセット） ---
	HRESULT hr = cmdContext_->MoveToNextFrame();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"DX12Manager::StartFrame - MoveToNextFrame 失敗 (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	// --- コマンドリスト取得（記録状態であること） ---
	ID3D12GraphicsCommandList* list = cmdContext_->GetList();
	if (!list) {
		Utils::Log(L"DX12Manager::StartFrame - コマンドリストが無効\n");
		return E_FAIL;
	}

	// --- SwapChain が指す現在のバックバッファを一度だけ取得して使い回す ---
	UINT currIndex = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = framebuf_->GetBackBuffer(currIndex);

	if (!backBuffer) {
		Utils::Log(std::format(L"DX12Manager::StartFrame - バックバッファが無効 (index={})\n", currIndex));
		return E_FAIL;
	}

	// --- tracked state（CPU側の追跡値）を取得してログ ---
	D3D12_RESOURCE_STATES tracked = framebuf_->GetBackBufferState(currIndex);

	// --- Present -> RenderTarget への遷移バリアを構築 ---
	D3D12_RESOURCE_BARRIER barrierToRT{};
	barrierToRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierToRT.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierToRT.Transition.pResource = backBuffer;
	barrierToRT.Transition.StateBefore = tracked;
	barrierToRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierToRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	if (barrierToRT.Transition.StateBefore != barrierToRT.Transition.StateAfter) {
		list->ResourceBarrier(1, &barrierToRT);
	}
	else {
		Utils::Log(L"StartFrame: skipped ResourceBarrier (already in target state)\n");
	}

	// --- CPU側の tracked state を更新（バリアの有無に関わらず） ---
	framebuf_->SetBackBufferState(currIndex, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// --- RTV/DSV のハンドル確認とバインド ---
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = framebuf_->GetRtvHandle(currIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = framebuf_->GetDsvHandle();

	list->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// --- 画面クリア ---
	framebuf_->ClearRenderTarget(list, currIndex, clearColor);
	framebuf_->ClearDepthStencil(list);

	return S_OK;
}

HRESULT DX12Manager::EndFrame()
{
	// --- 必須サブシステムチェック ---
	if (!cmdContext_ || !swapChain_ || !framebuf_) {
		Utils::Log(L"DX12Manager::EndFrame - 必要なサブシステムが存在しません\n");
		return E_POINTER;
	}

	// --- SwapChain が指す現在のバックバッファを一度だけ取得 ---
	UINT currIndex = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = framebuf_->GetBackBuffer(currIndex);

	if (!backBuffer) {
		Utils::Log(std::format(L"DX12Manager::EndFrame - バックバッファが無効 (index={})\n", currIndex));
		return E_FAIL;
	}

	// --- コマンドリスト取得 ---
	ID3D12GraphicsCommandList* list = cmdContext_->GetList();
	if (!list) {
		Utils::Log(L"DX12Manager::EndFrame - コマンドリストが無効\n");
		return E_FAIL;
	}

	// --- tracked state を取得してログ ---
	D3D12_RESOURCE_STATES tracked = framebuf_->GetBackBufferState(currIndex);

	// --- RenderTarget -> Present の遷移バリアを作り、ログを出して発行 ---
	D3D12_RESOURCE_BARRIER barrierToPresent{};
	barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierToPresent.Transition.pResource = backBuffer;
	barrierToPresent.Transition.StateBefore = tracked;
	barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrierToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	if (barrierToPresent.Transition.StateBefore != barrierToPresent.Transition.StateAfter) {
		list->ResourceBarrier(1, &barrierToPresent);
	}
	else {
		Utils::Log(L"EndFrame: skipped ResourceBarrier (already in target state)\n");
	}

	// --- tracked state を更新（バリアの有無に関わらず） ---
	framebuf_->SetBackBufferState(currIndex, D3D12_RESOURCE_STATE_PRESENT);

	// --- コマンド送信とフェンスシグナル ---
	HRESULT hr = cmdContext_->ExecuteAndSignal();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"DX12Manager::EndFrame - ExecuteAndSignal 失敗 (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	// --- 表示 ---
	hr = swapChain_->Present(1, 0);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"DX12Manager::EndFrame - Present 失敗 (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
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
