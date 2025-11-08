#include "ConstantBufferAllocator.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::Resource;

ConstantBufferAllocator::ConstantBufferAllocator()
{
    dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

void ConstantBufferAllocator::Init()
{
    assert(dx12Mgr_->GetDevice());
    totalSize_ = DefaultSize;

    // --- Upload Heap作成 ---
    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = totalSize_;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Utils::ThrowIfFailed(dx12Mgr_->GetDevice()->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadHeap_)
    ));

    // --- マップ ---
    Utils::ThrowIfFailed(uploadHeap_->Map(0, nullptr, reinterpret_cast<void**>(&cpuPtr_)));
    gpuBase_ = uploadHeap_->GetGPUVirtualAddress();
    currentOffset_ = 0;
}

void ConstantBufferAllocator::Reset()
{
    currentOffset_ = 0;
}

CBAllocation ConstantBufferAllocator::Allocate(size_t size)
{
    assert(uploadHeap_ && "ConstantBufferAllocator not initialized.");

    // 256バイト境界にアライン
    size_t alignedSize = (size + 255) & ~255;

    // 空き確認
    if (currentOffset_ + alignedSize > totalSize_) {
        throw std::runtime_error("ConstantBufferAllocator: out of memory in upload buffer");
    }

    CBAllocation alloc{};
    alloc.cpuPtr = reinterpret_cast<uint8_t*>(cpuPtr_) + currentOffset_;
    alloc.gpuAddress = gpuBase_ + currentOffset_;
    alloc.size = static_cast<UINT>(alignedSize);

    currentOffset_ += alignedSize;
    return alloc;
}