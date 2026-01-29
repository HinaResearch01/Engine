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
	assert(dx12Mgr_);
	ID3D12Device* device = dx12Mgr_->GetDevice();
	if (!device) return E_FAIL;

	HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	if (FAILED(hr)) return hr;

	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fenceEvent_) return HRESULT_FROM_WIN32(GetLastError());

	fenceValues_.fill(0);
	frameIndex_ = 0;
	return S_OK;
}

void FrameSync::BeginFrame()
{
	Wait(fenceValues_[frameIndex_]);
}

uint64_t  FrameSync::EndFrame()
{
	auto* ctx = dx12Mgr_->GetCommandContext();
	assert(ctx && ctx->GetQueue());

	// signal value を進める（frameIndexごとの期待値を増やす）
	const uint64_t nextValue = fenceValues_[frameIndex_] + 1;
	fenceValues_[frameIndex_] = nextValue;

	ctx->GetQueue()->Signal(fence_.Get(), nextValue);

	// 次フレームへ
	frameIndex_ = (frameIndex_ + 1) % kFrameCount;

	return nextValue;
}

void FrameSync::Wait(uint64_t fenceValue)
{
	if (!fence_ || !fenceEvent_) return;
	if (fenceValue == 0) return;

	const uint64_t completed = fence_->GetCompletedValue();
	if (completed >= fenceValue) return;

	fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
	WaitForSingleObject(fenceEvent_, INFINITE);
}
