#include "CommandContext.h"
#include "Utils/Logger/UtilsLog.h"
#include "DX12/DX12Manager.h"
#include "../Framebuf/Framebuffer.h"
#include <format>
#include <cassert>
#include <stdexcept>

using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;

CommandContext::CommandContext(DX12Manager* ptr)
{
	dx12Mgr_ = ptr;
}

CommandContext::~CommandContext()
{
	// GPU の処理が残っていれば待つ（安全にリソースを破棄するため）
	if (queue_.Get() && fence_.Get()) {
		const UINT64 value = ++globalFenceValue_;
		if (SUCCEEDED(queue_->Signal(fence_.Get(), value))) {
			if (fence_->GetCompletedValue() < value) {
				fence_->SetEventOnCompletion(value, fenceEvent_);
				WaitForSingleObject(fenceEvent_, INFINITE);
			}
		}
	}

	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}

	// ComPtr の Reset
	queue_.Reset();
	list_.Reset();
	fence_.Reset();
	for (auto& a : allocators_) a.Reset();
	allocators_.clear();
	fenceValues_.clear();
}

HRESULT CommandContext::Create()
{
	HRESULT hr = S_OK;

	hr = CreateQueue();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: CreateQueue failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	hr = CreateAllocators(frameCount_);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: CreateAllocators failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	hr = CreateList();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: CreateList failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	hr = CreateFence();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: CreateFence failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	return S_OK;
}

HRESULT CommandContext::ExecuteAndWait()
{
	if (!queue_.Get() || !list_.Get() || allocators_.empty() || !fence_.Get() || !fenceEvent_) {
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	// Close してから実行
	hr = list_->Close();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Warning: Close command list failed before execute (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
	}

	ID3D12CommandList* lists[] = { list_.Get() };
	queue_->ExecuteCommandLists(1, lists);

	const UINT64 signalValue = ++globalFenceValue_;
	hr = queue_->Signal(fence_.Get(), signalValue);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: queue->Signal failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	// すぐ待つ（同期）
	hr = fence_->SetEventOnCompletion(signalValue, fenceEvent_);
	if (FAILED(hr)) return hr;
	WaitForSingleObject(fenceEvent_, INFINITE);

	// Reset allocator/list for current frame after GPU done
	hr = allocators_[currentFrameIndex_]->Reset();
	if (FAILED(hr)) Utils::Log(std::format(L"Warning: allocator->Reset failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
	else {
		hr = list_->Reset(allocators_[currentFrameIndex_].Get(), nullptr);
		if (FAILED(hr)) Utils::Log(std::format(L"Warning: list_->Reset failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
	}

	return S_OK;
}

HRESULT CommandContext::ExecuteAndSignal()
{
	if (!queue_.Get() || !list_.Get() || allocators_.empty() || !fence_.Get()) {
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	// Close (無視可能)
	hr = list_->Close();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Warning: Close command list failed before execute (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
	}

	ID3D12CommandList* lists[] = { list_.Get() };
	queue_->ExecuteCommandLists(1, lists);

	// シグナルしてその値をこのフレーム用に記録
	const UINT64 signalValue = ++globalFenceValue_;
	hr = queue_->Signal(fence_.Get(), signalValue);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: queue->Signal failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	fenceValues_[currentFrameIndex_] = signalValue;

	// 呼び出し側は MoveToNextFrame() を呼んでフレームを進める
	return S_OK;
}

HRESULT CommandContext::MoveToNextFrame()
{
	if (!queue_.Get() || !fence_.Get() || allocators_.empty()) {
		return E_POINTER; // 必要なオブジェクトが存在しない
	}

	// 次フレームインデックスを算出
	UINT nextIndex = (currentFrameIndex_ + 1) % frameCount_;

	// GPU が該当アロケータを使い終わるまで待機
	const UINT64 expectedFence = fenceValues_[nextIndex];
	if (expectedFence != 0 && fence_->GetCompletedValue() < expectedFence) {
		HRESULT hr = fence_->SetEventOnCompletion(expectedFence, fenceEvent_);
		if (FAILED(hr)) return hr;
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// --- コマンドアロケータをリセット ---
	HRESULT hr = allocators_[nextIndex]->Reset();
	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"Warning: allocator->Reset failed for frame {} (hr=0x{:08X})\n",
			nextIndex, static_cast<unsigned>(hr)
		));
	}

	// --- コマンドリストをリセット ---
	hr = list_->Reset(allocators_[nextIndex].Get(), nullptr);
	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"Warning: list_->Reset failed for frame {} (hr=0x{:08X})\n",
			nextIndex, static_cast<unsigned>(hr)
		));
	}

	// --- ビューポート／シザー状態を初期化 ---
	ResetCachedRasterState();

	// 現在のフレームインデックスを更新
	currentFrameIndex_ = nextIndex;

	return S_OK;
}

HRESULT CommandContext::WaitForGpu()
{
	if (!fence_.Get() || !fenceEvent_) {
		return E_POINTER;
	}

	const UINT64 completed = fence_->GetCompletedValue();
	// globalFenceValue_ が 0 の場合はまだ Signal されていない可能性がある
	if (completed >= globalFenceValue_) {
		return S_OK;
	}

	HRESULT hr = fence_->SetEventOnCompletion(globalFenceValue_, fenceEvent_);
	if (FAILED(hr)) {
		return hr;
	}

	DWORD waitResult = WaitForSingleObject(fenceEvent_, INFINITE);
	if (waitResult != WAIT_OBJECT_0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

void CommandContext::SetViewport(const Viewport& vp)
{
	if (!list_) return;
	if (!viewportSet_ || currentViewport_ != vp) {
		D3D12_VIEWPORT d3dvp = vp.ToD3D();
		list_->RSSetViewports(1, &d3dvp);
		currentViewport_ = vp;
		viewportSet_ = true;
	}
}

void CommandContext::SetScissor(const Scissor& sc)
{
	if (!list_) return;
	if (!scissorSet_ || currentScissor_ != sc) {
		D3D12_RECT rect = sc.ToD3D();
		list_->RSSetScissorRects(1, &rect);
		currentScissor_ = sc;
		scissorSet_ = true;
	}
}

void CommandContext::SetFullViewportFromFramebuffer()
{
	if (!dx12Mgr_) return;

	Framebuffer* fb = dx12Mgr_->GetFramebuffer();
	if (!fb) return;

	// バックバッファ情報からサイズを取得
	UINT w = static_cast<UINT>(fb->GetBufferCount() > 0 ? fb->GetBackBuffer(0)->GetDesc().Width : 0);
	UINT h = static_cast<UINT>(fb->GetBufferCount() > 0 ? fb->GetBackBuffer(0)->GetDesc().Height : 0);

	if (w == 0 || h == 0) {
		return;
	}

	Viewport vp{};
	vp.TopLeftX = 0.f;
	vp.TopLeftY = 0.f;
	vp.Width = static_cast<float>(w);
	vp.Height = static_cast<float>(h);
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	SetViewport(vp);
}

void CommandContext::SetFullScissorFromFramebuffer()
{
	if (!dx12Mgr_) return;

	Framebuffer* fb = dx12Mgr_->GetFramebuffer();
	if (!fb) return;

	// サイズ取得
	UINT w = static_cast<UINT>(fb->GetBufferCount() > 0 ? fb->GetBackBuffer(0)->GetDesc().Width : 0); 
	UINT h = static_cast<UINT>(fb->GetBufferCount() > 0 ? fb->GetBackBuffer(0)->GetDesc().Height : 0);

	if (w == 0 || h == 0) return;

	Scissor sc{};
	sc.Left = 0; sc.Top = 0; sc.Right = static_cast<LONG>(w); sc.Bottom = static_cast<LONG>(h);

	SetScissor(sc);
}

HRESULT CommandContext::CreateQueue()
{
	HRESULT hr = S_OK;

	ID3D12Device* device = nullptr;
	if (dx12Mgr_) device = dx12Mgr_->GetDevice();
	if (!device) {
		return E_POINTER;
	}

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&queue_));

	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}

HRESULT CommandContext::CreateAllocators(UINT frameCount)
{
	HRESULT hr = S_OK;

	ID3D12Device* device = nullptr;
	if (dx12Mgr_) device = dx12Mgr_->GetDevice();
	if (!device) {
		return E_POINTER;
	}

	allocators_.resize(frameCount);
	fenceValues_.assign(frameCount, 0);
	frameCount_ = frameCount;
	currentFrameIndex_ = 0;

	for (UINT i = 0; i < frameCount; ++i) {
		hr = device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocators_[i]));
		if (FAILED(hr)) {
			return hr;
		}
	}

	return S_OK;
}

HRESULT CommandContext::CreateList()
{
	HRESULT hr = S_OK;

	ID3D12Device* device = nullptr;
	if (dx12Mgr_) device = dx12Mgr_->GetDevice();
	if (!device) {
		return E_POINTER;
	}

	hr = device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		allocators_[currentFrameIndex_].Get(),
		nullptr,
		IID_PPV_ARGS(&list_));

	if (FAILED(hr)) {
		return hr;
	}

	// CreateCommandList は "recording" 状態で返るので、ここで一旦 Close しておく（呼び出し側で Reset して再利用）
	hr = list_->Close();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Warning: Close initial command list failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
	}

	return S_OK;
}

HRESULT CommandContext::CreateFence()
{
	HRESULT hr = S_OK;

	ID3D12Device* device = nullptr;
	if (dx12Mgr_) device = dx12Mgr_->GetDevice();
	if (!device) {
		return E_POINTER;
	}

	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	if (FAILED(hr)) {
		return hr;
	}

	// イベントを作る
	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fenceEvent_) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	globalFenceValue_ = 0;
	// fenceValues_ は CreateAllocators で assign しているので、ここでは冗長に初期化しないか、確実にサイズを合わせる:
	if (fenceValues_.size() != frameCount_) {
		fenceValues_.assign(frameCount_, 0);
	}

	return S_OK;
}

void CommandContext::ResetCachedRasterState()
{
	viewportSet_ = false;
	scissorSet_ = false;

	// 明示的に初期値をクリア（デバッグ時の安全策）
	currentViewport_ = Viewport{};
	currentScissor_ = Scissor{};
}
