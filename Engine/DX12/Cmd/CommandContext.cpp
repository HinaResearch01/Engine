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

HRESULT Tsumi::DX12::CommandContext::ExecuteAndWait()
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

HRESULT Tsumi::DX12::CommandContext::ExecuteAndSignal()
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

HRESULT Tsumi::DX12::CommandContext::MoveToNextFrame()
{
	if (!queue_.Get() || !fence_.Get() || allocators_.empty()) {
		return E_POINTER;
	}

	// advance index
	UINT nextIndex = (currentFrameIndex_ + 1) % frameCount_;

	// この nextIndex の allocator を再利用する前に、GPU がそのフレームの作業を終えていることを確認
	const UINT64 expectedFence = fenceValues_[nextIndex];
	if (expectedFence != 0 && fence_->GetCompletedValue() < expectedFence) {
		// GPU が終わっていなければ待つ
		HRESULT hr = fence_->SetEventOnCompletion(expectedFence, fenceEvent_);
		if (FAILED(hr)) return hr;
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// allocator をクリアして、コマンドリストをその allocator で Reset しておく
	HRESULT hr = allocators_[nextIndex]->Reset();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Warning: allocator->Reset failed for frame {} (hr=0x{:08X})\n", nextIndex, static_cast<unsigned>(hr)));
		// 続行は試みる
	}

	hr = list_->Reset(allocators_[nextIndex].Get(), nullptr);
	if (FAILED(hr)) {
		Utils::Log(std::format(L"Warning: list_->Reset failed for frame {} (hr=0x{:08X})\n", nextIndex, static_cast<unsigned>(hr)));
		// 続行は試みる
	}

	// 現フレームを next に更新
	currentFrameIndex_ = nextIndex;

	return S_OK;
}

HRESULT Tsumi::DX12::CommandContext::WaitForGpu()
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

HRESULT Tsumi::DX12::CommandContext::CreateQueue()
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

HRESULT Tsumi::DX12::CommandContext::CreateAllocators(UINT frameCount)
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

	globalFenceValue_ = 0;
	for (UINT i = 0; i < frameCount_; ++i) fenceValues_[i] = 0;

	return S_OK;
}
