#include "FrameSync.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;
using namespace Microsoft::WRL;

FrameSync::FrameSync(DX12Manager* ptr) : dx12Mgr_(ptr) {}

FrameSync::~FrameSync()
{
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
	fence_.Reset();
}

HRESULT FrameSync::Init()
{
	if (!dx12Mgr_ || !dx12Mgr_->GetDevice()) return E_POINTER;

	bufferCount_ = dx12Mgr_->GetBufferCount();
	fenceValues_.assign(bufferCount_, 0);

	HRESULT hr = dx12Mgr_->GetDevice()->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	if (FAILED(hr)) return hr;

	fenceEvent_ = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fenceEvent_) return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

void FrameSync::BeginFrame(uint32_t frameIndex)
{
	frameIndex_ = frameIndex;

	const uint64_t fv = fenceValues_[frameIndex_];
	if (fv == 0) return;

	if (fence_->GetCompletedValue() < fv) {
		fence_->SetEventOnCompletion(fv, fenceEvent_);
		::WaitForSingleObject(fenceEvent_, INFINITE);
	}
}

void FrameSync::EndFrame(uint32_t frameIndex)
{
	auto* queue = dx12Mgr_->GetGraphicsQueue();
	const uint64_t next = fenceValues_[frameIndex] + 1;
	fenceValues_[frameIndex] = next;
	queue->Signal(fence_.Get(), next);
}

void FrameSync::Wait(uint64_t fenceValue)
{
	if (!fence_ || !fenceEvent_) return;
	if (fenceValue == 0) return;

	// GPUの完了値を確認
	if (fence_->GetCompletedValue() < fenceValue) {
		fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}
}
