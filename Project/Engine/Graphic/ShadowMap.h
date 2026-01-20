#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

namespace Tsumi::DX12 {

class DX12Manager;

class ShadowMap {
public:
    ShadowMap(DX12Manager* dx12);
    ~ShadowMap();

    /// <summary>
    /// 初期化（リソース生成）
    /// </summary>
    /// <param name="width">解像度幅</param>
    /// <param name="height">解像度高さ</param>
    void Init(uint32_t width = 2048, uint32_t height = 2048);

    /// <summary>
    /// DSVハンドル取得（書き込み用）
    /// </summary>
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

    /// <summary>
    /// SRVハンドル取得（読み込み用）
    /// </summary>
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const;

    /// <summary>
    /// リソースステート遷移
    /// </summary>
    void Transition(D3D12_RESOURCE_STATES nextState);

    /// <summary>
    /// 深度クリア
    /// </summary>
    void Clear(ID3D12GraphicsCommandList* cmdList);

    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }
    ID3D12Resource* GetResource() const { return resource_.Get(); }

private:
    void CreateResource();
    void CreateViews();

private:
    DX12Manager* dx12_ = nullptr;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    
    // SRVはDX12ManagerのPersistentHeapに確保する想定
    // または独自にHeapを持つか。今回は管理を楽にするため、SRV用ディスクリプタは
    // PersistentHeapから貰ってくる形にするか、あるいはメンバで持つか。
    // 設計に合わせて、ここではシンプルに自身のHeapを持つ形にするか、
    // あるいはEngineの設計思想（DescriptorAllocator使用）に合わせる。
    // 既存コードを見る限り PersistentDescAlloc があるので、そこから割り当てるのが行儀が良い。
    // しかし管理が複雑になるため、当面はメンバ変数としてSRVインデックスまたはハンドルを保持する。
    
    // 簡易実装として、SRVもここ（メンバ）で持つことができない（ShaderVisibleなHeapが必要なため）。
    // そのため、DX12ManagerのPersistentAllocatorから確保したIDを保持する。
    uint32_t srvDescriptorIndex_ = 0; 
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};

    uint32_t width_ = 2048;
    uint32_t height_ = 2048;

    D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_COMMON;
};

}
