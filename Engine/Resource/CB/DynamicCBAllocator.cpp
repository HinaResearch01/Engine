#include "DynamicCBAllocator.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::Resource;
using namespace Microsoft::WRL;

DynamicCBAllocator::DynamicCBAllocator()
{
    dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

void DynamicCBAllocator::Init(uint32_t totalSizePerframe)
{
    assert(dx12Mgr_->GetDevice());
    totalSizePerFrame_ = Align256(totalSizePerframe);

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = totalSizePerFrame_;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    for (uint32_t i = 0; i < kFrameCount; ++i)
    {
        HRESULT hr = dx12Mgr_->GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadHeaps_[i])
        );
        assert(SUCCEEDED(hr));

        // CPUから書き込み可能にしておく（Mapは一度だけでOK）
        hr = uploadHeaps_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedPtrs_[i]));
        assert(SUCCEEDED(hr));
    }
}

void DynamicCBAllocator::BeginFrame(uint32_t frameIndex)
{
    assert(frameIndex < kFrameCount);
    currentFrame_ = frameIndex;
    currentOffset_ = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS DynamicCBAllocator::Allocate(const void* srcData, uint32_t size)
{
    assert(size > 0);
    uint32_t alignedSize = Align256(size);

    // バッファオーバーラン防止
    assert(currentOffset_ + alignedSize <= totalSizePerFrame_ && "DynamicCBAllocator overflow!");

    // 書き込み先ポインタ
    uint8_t* dst = mappedPtrs_[currentFrame_] + currentOffset_;
    memcpy(dst, srcData, size);

    // GPU仮想アドレスを計算
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddr =
        uploadHeaps_[currentFrame_]->GetGPUVirtualAddress() + currentOffset_;

    currentOffset_ += alignedSize;
    return gpuAddr;
}
