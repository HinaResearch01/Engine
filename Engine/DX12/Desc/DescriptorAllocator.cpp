#include "DescriptorAllocator.h"
#include "DX12/DX12Manager.h"
#include "Utils/Logger/UtilsLog.h"

using namespace Tsumi::DX12;

DescriptorAllocator::DescriptorAllocator()
{
    dx12Mgr_ = DX12Manager::GetInstance();
}

HRESULT DescriptorAllocator::Init()
{
    D3D12_DESCRIPTOR_HEAP_TYPE type =
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    UINT numDescriptors = 1024;
    bool shaderVisible = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (!dx12Mgr_->GetDevice()) return E_POINTER;
    if (numDescriptors == 0) return E_INVALIDARG;

    heapType_ = type;
    shaderVisible_ = shaderVisible;
    capacity_ = numDescriptors;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = type;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    HRESULT hr = dx12Mgr_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"DescriptorAllocator::Init - CreateDescriptorHeap failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr;
    }

    descriptorSize_ = dx12Mgr_->GetDevice()->GetDescriptorHandleIncrementSize(type);
    used_.assign(numDescriptors, 0);

    return S_OK;
}

DescAlloc DescriptorAllocator::Allocate(UINT count)
{
    DescAlloc alloc{};
    if (!heap_) return alloc;
    if (count == 0 || count > capacity_) return alloc;

    std::lock_guard lock(mutex_);

    // シンプルなファーストフィット検索（連続領域）
    UINT run = 0;
    UINT start = UINT_MAX;
    for (UINT i = 0; i < capacity_; ++i) {
        if (used_[i] == 0) {
            if (run == 0) start = i;
            ++run;
            if (run == count) {
                // mark used
                for (UINT j = start; j < start + count; ++j) used_[j] = 1;

                // compute CPU/GPU handles
                D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = heap_->GetCPUDescriptorHandleForHeapStart();
                cpuStart.ptr = SIZE_T(cpuStart.ptr) + SIZE_T(start) * SIZE_T(descriptorSize_);

                D3D12_GPU_DESCRIPTOR_HANDLE gpuStart{};
                if (shaderVisible_) {
                    gpuStart = heap_->GetGPUDescriptorHandleForHeapStart();
                    gpuStart.ptr = SIZE_T(gpuStart.ptr) + SIZE_T(start) * SIZE_T(descriptorSize_);
                }

                alloc.cpuHandle = cpuStart;
                alloc.gpuHandle = gpuStart;
                alloc.startIndex = start;
                alloc.descriptorCount = count;
                return alloc;
            }
        }
        else {
            run = 0;
            start = UINT_MAX;
        }
    }

    // 見つからなかった
    return alloc;
}

void DescriptorAllocator::Reset()
{
    std::lock_guard lock(mutex_);
    std::fill(used_.begin(), used_.end(), static_cast<uint8_t>(0));
}