#include "CommandContext.h"
#include "Utils/Logger/Logger.h"
#include "DX12/DX12Manager.h"
#include "../Framebuf/Framebuffer.h"
#include <format>
#include <cassert>
#include <stdexcept>

using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;

CommandContext::CommandContext(DX12Manager* ptr, D3D12_COMMAND_LIST_TYPE type)
	: dx12Mgr_(ptr), listType_(type)
{}

CommandContext::~CommandContext()
{
	if (queue_ && flushFence_ && flushEvent_) {
		WaitForGpu();
	}

	if (flushEvent_) {
		CloseHandle(flushEvent_);
		flushEvent_ = nullptr;
	}

	list_.Reset();
	for (auto& a : allocators_) a.Reset();
	allocators_.clear();
	queue_.Reset();
	flushFence_.Reset();
}

HRESULT CommandContext::Create(UINT frameCount)
{
	frameCount_ = frameCount;

	HRESULT hr = CreateQueue();
	if (FAILED(hr)) return hr;

	hr = CreateAllocators(frameCount_);
	if (FAILED(hr)) return hr;

	hr = CreateList();
	if (FAILED(hr)) return hr;

	hr = CreateFlushFence();
	if (FAILED(hr)) return hr;

	return S_OK;
}

HRESULT CommandContext::ResetForFrame(UINT frameIndex)
{
	if (!list_ || !queue_ || allocators_.empty()) return E_POINTER;
	if (frameIndex >= allocators_.size()) return E_INVALIDARG;

	currentFrameIndex_ = frameIndex;

	// 「Open/Closeの状態」は isListOpen_ で一元管理
	if (isListOpen_) {
		HRESULT hrClose = list_->Close();
		if (FAILED(hrClose)) {
			Utils::Logger::Error("CommandContext::ResetForFrame - Close failed (hr=0x{:08X})\n",
								 (unsigned)hrClose);
			return hrClose;
		}
		isListOpen_ = false;
	}

	HRESULT hr = allocators_[frameIndex]->Reset();
	if (FAILED(hr)) {
		Utils::Logger::Error("CommandContext::ResetForFrame - allocator Reset failed (hr=0x{:08X})\n",
							 (unsigned)hr);
		return hr;
	}

	hr = list_->Reset(allocators_[frameIndex].Get(), nullptr);
	if (FAILED(hr)) {
		Utils::Logger::Error("CommandContext::ResetForFrame - list Reset failed (hr=0x{:08X})\n",
							 (unsigned)hr);
		return hr;
	}

	isListOpen_ = true;
	ResetCachedRasterState();
	return S_OK;
}

HRESULT CommandContext::Execute()
{
	if (!queue_ || !list_) return E_POINTER;

	// Closeはここだけ
	if (isListOpen_) {
		HRESULT hr = list_->Close();
		if (FAILED(hr)) {
			Utils::Logger::Error("CommandContext::Execute - Close failed (hr=0x{:08X})\n",
								 (unsigned)hr);
			return hr;
		}
		isListOpen_ = false;
	}

	ID3D12CommandList* lists[] = { list_.Get() };
	queue_->ExecuteCommandLists(1, lists);
	return S_OK;
}

HRESULT CommandContext::WaitForGpu()
{
	if (!queue_ || !flushFence_ || !flushEvent_) return E_POINTER;

	HRESULT hr = SignalFlush();
	if (FAILED(hr)) return hr;

	return WaitFlush(flushValue_);
}

void CommandContext::SetViewport(const Viewport& vp)
{
	if (!list_) return;
	if (!isListOpen_) return;
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
	if (!isListOpen_) return;
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
	auto* fb = dx12Mgr_->GetFramebuffer();
	if (!fb) return;

	const UINT w = (UINT)fb->GetWidth();
	const UINT h = (UINT)fb->GetHeight();
	if (w == 0 || h == 0) return;

	Viewport vp{};
	vp.TopLeftX = 0.f; vp.TopLeftY = 0.f;
	vp.Width = (float)w; vp.Height = (float)h;
	vp.MinDepth = 0.f; vp.MaxDepth = 1.f;
	SetViewport(vp);
}

void CommandContext::SetFullScissorFromFramebuffer()
{
	if (!dx12Mgr_) return;
	auto* fb = dx12Mgr_->GetFramebuffer();
	if (!fb) return;

	const UINT w = (UINT)fb->GetWidth();
	const UINT h = (UINT)fb->GetHeight();
	if (w == 0 || h == 0) return;

	Scissor sc{};
	sc.Left = 0; sc.Top = 0;
	sc.Right = (LONG)w; sc.Bottom = (LONG)h;
	SetScissor(sc);
}

HRESULT CommandContext::BeginOneShot()
{
	if (oneShotFenceValue_ != 0) {
		HRESULT hr = WaitFlush(oneShotFenceValue_);
		if (FAILED(hr)) return hr;
	}
	return ResetForFrame(0);
}

HRESULT CommandContext::EndOneShot()
{
	HRESULT hr = Execute();
	if (FAILED(hr)) return hr;

	hr = SignalFlush();
	if (FAILED(hr)) return hr;

	oneShotFenceValue_ = flushValue_;
	return S_OK;
}

HRESULT CommandContext::EndOneShotAndWait()
{
	HRESULT hr = Execute();
	if (FAILED(hr)) return hr;

	hr = SignalFlush();
	if (FAILED(hr)) return hr;

	oneShotFenceValue_ = flushValue_;
	return WaitFlush(oneShotFenceValue_);
}

void CommandContext::SetDescriptorHeaps(uint32_t count, ID3D12DescriptorHeap* const* heaps)
{
	if (!list_) return;
	assert(isListOpen_ && "SetDescriptorHeaps on CLOSED list");
	list_->SetDescriptorHeaps(count, heaps);
}

void CommandContext::SetGraphicsRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE table)
{
	if (!list_) return;
	assert(isListOpen_ && "SetGraphicsRootDescriptorTable on CLOSED list");
	list_->SetGraphicsRootDescriptorTable(rootIndex, table);
}

void CommandContext::SetGraphicsRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS va)
{
	if (!list_) return;
	assert(isListOpen_ && "SetGraphicsRootConstantBufferView on CLOSED list");
	list_->SetGraphicsRootConstantBufferView(rootIndex, va);
}

HRESULT CommandContext::CreateQueue()
{
	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	if (!device) return E_POINTER;

	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = listType_;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	return device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue_));
}

HRESULT CommandContext::CreateAllocators(UINT frameCount)
{
	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	if (!device) return E_POINTER;

	frameCount_ = (frameCount == 0) ? 1 : frameCount;
	allocators_.resize(frameCount_);

	for (uint32_t i = 0; i < frameCount_; ++i) {
		HRESULT hr = device->CreateCommandAllocator(listType_, IID_PPV_ARGS(&allocators_[i]));
		if (FAILED(hr)) return hr;
	}
	return S_OK;
}

HRESULT CommandContext::CreateList()
{
	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	if (!device) return E_POINTER;
	if (allocators_.empty()) return E_FAIL;

	// CreateCommandList は「open状態」で返る
	HRESULT hr = device->CreateCommandList(
		0, listType_, allocators_[0].Get(), nullptr, IID_PPV_ARGS(&list_));
	if (FAILED(hr)) return hr;

	isListOpen_ = true;
	currentFrameIndex_ = 0;
	ResetCachedRasterState();

	// ★最初のフレーム開始で ResetForFrame を必ず呼ぶ設計なので、
	// ここで一旦 close して closed にしておくと状態が一貫する（おすすめ）
	hr = list_->Close();
	if (FAILED(hr)) return hr;
	isListOpen_ = false;

	return S_OK;
}

HRESULT CommandContext::CreateFlushFence()
{
	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	if (!device) return E_POINTER;

	HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&flushFence_));
	if (FAILED(hr)) return hr;

	flushEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!flushEvent_) return HRESULT_FROM_WIN32(GetLastError());

	flushValue_ = 0;
	return S_OK;
}

HRESULT CommandContext::SignalFlush()
{
	++flushValue_;
	return queue_->Signal(flushFence_.Get(), flushValue_);
}

HRESULT CommandContext::WaitFlush(uint64_t value)
{
	if (flushFence_->GetCompletedValue() >= value) return S_OK;

	HRESULT hr = flushFence_->SetEventOnCompletion(value, flushEvent_);
	if (FAILED(hr)) return hr;

	DWORD wr = WaitForSingleObject(flushEvent_, INFINITE);
	if (wr != WAIT_OBJECT_0) return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}

void CommandContext::ResetCachedRasterState()
{
	viewportSet_ = false;
	scissorSet_ = false;
	currentViewport_ = Viewport{};
	currentScissor_ = Scissor{};
}
