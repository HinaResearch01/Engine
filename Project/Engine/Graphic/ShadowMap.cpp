#include "ShadowMap.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/Logger.h"

using namespace Tsumi::DX12;

ShadowMap::ShadowMap(DX12Manager* dx12)
    : dx12_(dx12)
{
}

ShadowMap::~ShadowMap()
{
    // SRVの解放が必要ならここで行う
    // dx12_->GetPersistentDescAlloc()->Free(srvDescriptorIndex_);
}

void ShadowMap::Init(uint32_t width, uint32_t height)
{
    width_ = width;
    height_ = height;

    CreateResource();
    CreateViews();
}

void ShadowMap::CreateResource()
{
    // ヒーププロパティ
    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    // リソース設定
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = width_;
    resDesc.Height = height_;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R32_TYPELESS; // SRVはR32_FLOAT, DSVはD32_FLOAT
    resDesc.SampleDesc.Count = 1;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // クリア値（深度1.0）
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    auto device = dx12_->GetDevice();
    auto hr = device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        currentState_,
        &clearValue,
        IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr)) {
        Tsumi::Utils::Logger::Error("Failed to create ShadowMap resource");
        throw std::runtime_error("ShadowMap Create Error");
    }
}

void ShadowMap::CreateViews()
{
    auto device = dx12_->GetDevice();

    // --- DSVの作成 ---
    // DSV用ヒープを作成（単体）
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf()));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(resource_.Get(), &dsvDesc, dsvHeap_->GetCPUDescriptorHandleForHeapStart());

    // --- SRVの作成 ---
    // PersistentAllocatorから確保
    auto* alloc = dx12_->GetPersistentDescAlloc();
    auto allocation = alloc->Allocate(1);
    
    srvCpuHandle_ = allocation.cpuHandle;
    srvGpuHandle_ = allocation.gpuHandle;
    srvDescriptorIndex_ = allocation.index; // 管理用

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // 赤成分として深度を読む
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    device->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDsvHandle() const
{
    return dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetSrvHandle() const
{
    return srvGpuHandle_;
}

void ShadowMap::Transition(D3D12_RESOURCE_STATES nextState)
{
    if (currentState_ == nextState) return;

    // DX12Managerからコマンドリストを取得してバリアを張るのが理想だが、
    // 呼び出し側でCommandContextを持っているはずなので、インタフェースを変えるか
    // あるいはここでDX12Manager経由で最新のリストを取得するか。
    // ShadowMap::Transition(CommandContext& cmd, ...) の方が綺麗だが、
    // ここではDX12ManagerがSingleton的かつメンバにあるのでそれを使う。
    
    auto* cmd = dx12_->GetCommandContext()->GetList();
    
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource_.Get();
    barrier.Transition.StateBefore = currentState_;
    barrier.Transition.StateAfter = nextState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    cmd->ResourceBarrier(1, &barrier);
    currentState_ = nextState;
}

void ShadowMap::Clear(ID3D12GraphicsCommandList* cmdList)
{
    // クリア時はDEPTH_WRITEである必要がある
    if (currentState_ != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
        // 必要なら遷移させるが、通常呼び出し側で制御する
    }

    auto dsv = GetDsvHandle();
    cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}
