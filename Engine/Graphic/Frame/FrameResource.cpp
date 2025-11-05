#include "FrameResource.h"
#include "DX12/DX12Manager.h"
#include <stdexcept>
#include <cassert>
#include <d3dx12.h> 

using namespace Tsumi::Graphic;
using Microsoft::WRL::ComPtr;

FrameResource::FrameResource() 
{
    dx12Mgr_ = DX12::DX12Manager::GetInstance();
    descAlloc_ = DX12::DescriptorAllocator::GetInstance();
}

bool FrameResource::Init() 
{
    size_t uploadBufferSize = static_cast<size_t>(16) * 1024;

    if (!dx12Mgr_->GetDevice()) return false;
    if (!descAlloc_)
        return false;  // DescriptorAllocator は外部で初期化済みを期待

    // コマンドアロケータ生成
    HRESULT hr = dx12Mgr_->GetDevice()->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                IID_PPV_ARGS(&cmdAllocator_));
    if (FAILED(hr) || !cmdAllocator_) {
        return false;
    }

    // アップロードバッファ生成（UPLOAD ヒープ）
    uploadBufferSize_ = uploadBufferSize;
    mappedUploadPtr_ = nullptr;
    if (uploadBufferSize_ > 0) {
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        // バッファサイズは 1 の倍数で良い。システム側で CB サイズを 256
        // の倍数に整える想定
        desc.Width = uploadBufferSize_;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        hr = dx12Mgr_->GetDevice()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&uploadBuffer_));
        if (FAILED(hr) || !uploadBuffer_) {
            uploadBuffer_.Reset();
            return false;
        }

        // 常時マップしておく
        CD3DX12_RANGE readRange(0, 0);  // CPU 側は読み取らない
        hr = uploadBuffer_->Map(0, &readRange,
                                reinterpret_cast<void**>(&mappedUploadPtr_));
        if (FAILED(hr)) {
            uploadBuffer_.Reset();
            mappedUploadPtr_ = nullptr;
            return false;
        }
    }

    fenceValue_ = 0;
    return true;
}

bool Tsumi::Graphic::FrameResource::ResetForRecord() 
{
    // CommandAllocator をリセットしてコマンドリストをこの allocator で再セット
    if (!cmdAllocator_) return false;
    if (!dx12Mgr_->GetCmdList()) return false;

    HRESULT hr = dx12Mgr_->GetCmdList()->Reset(cmdAllocator_.Get(), nullptr);
    if (FAILED(hr)) return false;

    // DescriptorAllocator のリセット
    if (descAlloc_) {
        
        descAlloc_->Reset();
    }

    return true;
}
