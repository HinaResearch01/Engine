#include "CommandContext.h"
#include "Utils/Logger/UtilsLog.h"
#include "DX12/DX12Manager.h"
#include <format>
#include <cassert>
#include <stdexcept>

using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;

CommandContext::CommandContext()
{
	dx12Mgr_ = DX12Manager::GetInstance();
}

CommandContext::~CommandContext()
{
	// GPU の処理が残っていれば待つ（安全にリソースを破棄するため）
	if (queue_.Get() && fence_.Get()) {
		// Signal with next fence value and wait
		const UINT64 value = ++fenceValue_;
		HRESULT hr = queue_->Signal(fence_.Get(), value);
		if (SUCCEEDED(hr)) {
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
	allocator_.Reset();
	list_.Reset();
	fence_.Reset();
}

HRESULT CommandContext::Create()
{
	HRESULT hr = S_OK;

	hr = CreateQueue();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: CreateQueue failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	hr = CreateAllocator();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: CreateAllocator failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
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

HRESULT Tsumi::DX12::CommandContext::ExecuteAndWait()
{
	if (!queue_.Get() || !list_.Get() || !allocator_.Get() || !fence_.Get() || !fenceEvent_) {
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	// コマンドリストを閉じる（既に閉じている場合は無視）
	hr = list_->Close();
	if (FAILED(hr)) {
		// Close に失敗してもログは出すが続行を試みる
		Utils::Log(std::format(L"Warning: Close command list failed before execute (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
	}

	// Execute
	ID3D12CommandList* lists[] = { list_.Get() };
	queue_->ExecuteCommandLists(1, lists);

	// Signal
	const UINT64 signalValue = ++fenceValue_;
	hr = queue_->Signal(fence_.Get(), signalValue);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: queue->Signal failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	// Wait
	hr = WaitForGpu();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Error: WaitForGpu failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		return hr;
	}

	// コマンドリストを再利用できるように Reset を試みる（呼び出し側が Reset を行う設計ならこの部分は不要）
	hr = allocator_->Reset();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Warning: allocator->Reset failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
		// 続行
	}
	else {
		hr = list_->Reset(allocator_.Get(), nullptr); // nullptr の代わりに初期 PSO を渡す場合はここを変更
		if (FAILED(hr)) {
			Utils::Log(std::format(L"Warning: list_->Reset failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
			// 続行
		}
	}

	return S_OK;
}

HRESULT Tsumi::DX12::CommandContext::WaitForGpu()
{
	if (!fence_.Get() || !fenceEvent_) {
		return E_POINTER;
	}

	const UINT64 completed = fence_->GetCompletedValue();
	// fenceValue_ が 0 の場合はまだ Signal されていない可能性がある
	if (completed >= fenceValue_) {
		return S_OK;
	}

	// 待機対象は fenceValue_
	HRESULT hr = S_OK;
	hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
	if (FAILED(hr)) {
		return hr;
	}

	DWORD waitResult = WaitForSingleObject(fenceEvent_, INFINITE);
	if (waitResult != WAIT_OBJECT_0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

HRESULT Tsumi::DX12::CommandContext::CreateQueue()
{
	HRESULT hr = S_OK;

	// 安全のため device の有無をチェック
	ID3D12Device* device = nullptr;
	if (dx12Mgr_) device = dx12Mgr_->GetDevice();
	if (!device) {
		return E_POINTER;
	}

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	// 明示的に設定（デフォルトで DIRECT になるが明記しておく）
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

HRESULT Tsumi::DX12::CommandContext::CreateAllocator()
{
	HRESULT hr = S_OK;

	ID3D12Device* device = nullptr;
	if (dx12Mgr_) device = dx12Mgr_->GetDevice();
	if (!device) {
		return E_POINTER;
	}

	hr = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&allocator_));

	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}

HRESULT Tsumi::DX12::CommandContext::CreateList()
{
	HRESULT hr = S_OK;

	ID3D12Device* device = nullptr;
	if (dx12Mgr_) device = dx12Mgr_->GetDevice();
	if (!device) {
		return E_POINTER;
	}

	hr = device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		allocator_.Get(),
		nullptr,
		IID_PPV_ARGS(&list_));

	if (FAILED(hr)) {
		return hr;
	}

	// CreateCommandList は "recording" 状態で返るので
	// Reset/Close の前提に合わせてここで一旦 Close しておく。
	hr = list_->Close();
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}

HRESULT Tsumi::DX12::CommandContext::CreateFence()
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

	// 初期フェンス値
	fenceValue_ = 0;

	return S_OK;
}
