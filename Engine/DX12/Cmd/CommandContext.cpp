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
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = dx12Mgr_->GetDevice()->CreateCommandQueue(
		&commandQueueDesc,
		IID_PPV_ARGS(&queue_));

	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}

HRESULT Tsumi::DX12::CommandContext::CreateAllocator()
{
	HRESULT hr = S_OK;
	hr = dx12Mgr_->GetDevice()->CreateCommandAllocator(
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
	hr = dx12Mgr_->GetDevice()->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		allocator_.Get(),
		nullptr,
		IID_PPV_ARGS(&list_));

	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}
