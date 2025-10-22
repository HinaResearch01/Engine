#include "CommandContext.h"
#include "Utils/Logger/UtilsLog.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;

CommandContext::CommandContext()
{
	dx12Mgr_ = DX12Manager::GetInstance();
}

CommandContext::~CommandContext()
{
	queue_.Reset();
	allocator_.Reset();
	list_.Reset();
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
