#include "FrameSync.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;
using namespace Microsoft::WRL;

FrameSync::FrameSync(DX12Manager* ptr)
{
    dx12Mgr_ = ptr;
}

HRESULT FrameSync::Init()
{
    assert(dx12Mgr_);
    ID3D12Device* device = dx12Mgr_->GetDevice();
    if (!device) {
        return E_FAIL;
    }

    // Fence作成
    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    if (FAILED(hr)) {
        return hr;
    }

    // イベント作成
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent_ == nullptr) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    fenceValues_.fill(0);
    frameIndex_ = 0;

    return S_OK;
}

void FrameSync::BeginFrame()
{
    assert(dx12Mgr_->GetCommandContext()->GetQueue());

    // 現フレームのFence値がGPUにより完了済みかを確認
    const uint64_t completed = fence_->GetCompletedValue();
    if (completed < fenceValues_[frameIndex_])
    {
        // GPUがまだ処理中なら待機
        fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
}

void FrameSync::EndFrame()
{
    assert(dx12Mgr_->GetCommandContext()->GetQueue());

    // 次に期待するFence値を設定
    const uint64_t nextValue = fenceValues_[frameIndex_] + 1;
    fenceValues_[frameIndex_] = nextValue;

    // GPUにSignalを発行
    dx12Mgr_->GetCommandContext()->GetQueue()->Signal(fence_.Get(), nextValue);

    // フレーム番号をローテーション
    frameIndex_ = (frameIndex_ + 1) % kFrameCount;
}