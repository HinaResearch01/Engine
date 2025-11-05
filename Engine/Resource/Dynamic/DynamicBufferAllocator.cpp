#include "DynamicBufferAllocator.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::Resource;
using Microsoft::WRL::ComPtr;

DynamicBufferAllocator::DynamicBufferAllocator()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

DynamicBufferAllocator::~DynamicBufferAllocator()
{
    if (buffer_) {
        buffer_->Unmap(0, nullptr);
        buffer_.Reset();
        mappedBegin_ = nullptr;
    }
}

HRESULT DynamicBufferAllocator::Init()
{
    size_t bufferSize = 0;
    bufferSize_ = bufferSize;
    currentOffset_ = 0;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = bufferSize_;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = dx12Mgr_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&buffer_));
    if (FAILED(hr)) {
        return hr;
    }

    // 永続マップ
    hr = buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBegin_));
    if (FAILED(hr)) {
        buffer_.Reset();
        mappedBegin_ = nullptr;
        return hr;
    }

    return S_OK;
}

DynamicBufferAllocator::Allocation DynamicBufferAllocator::Allocate(size_t size)
{
    Allocation result{};
    if (!buffer_ || !mappedBegin_) {
        return result;
    }

    size_t alignedSize = Align256(size);
    if (currentOffset_ + alignedSize > bufferSize_) {
        // バッファ不足（とりあえず assert にしておく）
        assert(false && "DynamicBufferAllocator out of memory for this frame.");
        return result;
    }

    uint8_t* cpuPtr = mappedBegin_ + currentOffset_;
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = buffer_->GetGPUVirtualAddress() + currentOffset_;

    result.cpuPtr = cpuPtr;
    result.gpuAddress = gpuAddr;
    result.size = static_cast<UINT>(alignedSize);

    currentOffset_ += alignedSize;
    return result;
}