#include "TextureManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include <DirectXTex.h>

using namespace Tsumi::Resource;
using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

static DXGI_FORMAT MakeTypelessIfNeeded(DXGI_FORMAT fmt)
{
    switch (fmt) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        default:
            return fmt;
    }
}

TextureManager::TextureManager()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

void TextureManager::Emplace(const std::string& name, std::unique_ptr<Texture> tex)
{
    if (!tex) return;
    std::lock_guard lock(mutex_);
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        if (dx12Mgr_) {
            DescriptorAllocator* allocator = dx12Mgr_->GetPersistentDescAlloc();
            if (allocator && it->second && it->second->srvDesc.valid()) {
                uint32_t frameIndex = 0;
                if (dx12Mgr_->GetFrameSync()) frameIndex = dx12Mgr_->GetFrameSync()->GetFrameIndex();
                allocator->DeferFree(it->second->srvDesc, frameIndex);
            }
        }
    }
    textures_[name] = std::move(tex);
}

HRESULT TextureManager::CreateFromScratchImage(const std::string& name, const DirectX::ScratchImage& mipChain, DXGI_FORMAT viewFormat)
{
    // 画像データが存在しない場合はエラー
    if (mipChain.GetImageCount() == 0) {
        Utils::Log(std::format(
            L"[TextureManager] CreateFromScratchImage - 画像が空です: '{}'\n",
            Utils::Utf8ToWstring(name)));
        return E_INVALIDARG;
    }

    // DX12Manager の存在チェック
    if (!dx12Mgr_) {
        Utils::Log(L"[TextureManager] DX12Manager が存在しません\n");
        return E_FAIL;
    }

    // 必要なサブシステム取得
    ID3D12Device* device = dx12Mgr_->GetDevice();
    CommandContext* cmdCtx = dx12Mgr_->GetCommandContext();
    DescriptorAllocator* allocator = dx12Mgr_->GetPersistentDescAlloc();
    if (!device || !cmdCtx || !allocator) {
        Utils::Log(L"[TextureManager] DX12 の必要サブシステムが揃っていません\n");
        return E_FAIL;
    }

    // ScratchImage のメタデータ取得
    const TexMetadata& meta = mipChain.GetMetadata();
    UINT width = static_cast<UINT>(meta.width);
    UINT height = static_cast<UINT>(meta.height);
    UINT mipCount = static_cast<UINT>(meta.mipLevels);
    if (mipCount == 0) mipCount = 1;

    DXGI_FORMAT resourceFormat = meta.format;
    bool viewIsSRGB = false;
    switch (viewFormat) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            viewIsSRGB = true;
            break;
        default:
            break;
    }
    if (viewIsSRGB) {
        resourceFormat = MakeTypelessIfNeeded(meta.format);
    }

    // GPU リソース の作成
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = meta.width;
    desc.Height = static_cast<UINT>(meta.height);
    desc.DepthOrArraySize = static_cast<UINT16>(meta.arraySize);
    desc.MipLevels = static_cast<UINT16>(meta.mipLevels);
    desc.Format = resourceFormat;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> texture;
    CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device->CreateCommittedResource(
        &heap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture));

    if (FAILED(hr)) {
        Utils::Log(std::format(
            L"[TextureManager] default ヒープテクスチャ作成失敗 (hr=0x{:08X}) '{}'\n",
            static_cast<unsigned>(hr), Utils::Utf8ToWstring(name)));
        return hr;
    }

    // mipChain からサブリソース配列を生成する
    std::vector<D3D12_SUBRESOURCE_DATA> subres;
    PrepareUpload(device, mipChain.GetImages(), mipChain.GetImageCount(), meta, subres);

    // アップロードバッファの必要サイズを計算
    UINT64 uploadBytes = GetRequiredIntermediateSize(
        texture.Get(), 0, static_cast<UINT>(subres.size()));

    ComPtr<ID3D12Resource> upload;
    CD3DX12_HEAP_PROPERTIES upHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);
    hr = device->CreateCommittedResource(
        &upHeap,
        D3D12_HEAP_FLAG_NONE,
        &upDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload));

    if (FAILED(hr)) {
        Utils::Log(std::format(
            L"[TextureManager] upload ヒープ作成失敗 (hr=0x{:08X}) '{}'\n",
            static_cast<unsigned>(hr), Utils::Utf8ToWstring(name)));
        return hr;
    }

    // UpdateSubresources で GPU へアップロード
    ID3D12GraphicsCommandList* list = cmdCtx->GetList();
    if (!list) {
        Utils::Log(L"[TextureManager] CommandList が null です\n");
        return E_FAIL;
    }

    UpdateSubresources(
        list,
        texture.Get(),
        upload.Get(),
        0, 0,
        static_cast<UINT>(subres.size()),
        subres.data()
    );

    // コピー完了後、PS シェーダ用の読み取り状態へ遷移
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    list->ResourceBarrier(1, &barrier);

    // GPU 実行を待つ
    hr = cmdCtx->ExecuteAndWait();
    if (FAILED(hr)) {
        Utils::Log(std::format(
            L"[TextureManager] ExecuteAndWait 失敗 (hr=0x{:08X}) '{}'\n",
            static_cast<unsigned>(hr), Utils::Utf8ToWstring(name)));
        return hr;
    }

    // SRV (Shader Resource View) 作成
    DescAlloc descAlloc = allocator->Allocate(1);
    if (!descAlloc.valid()) {
        Utils::Log(std::format(
            L"[TextureManager] ディスクリプタ割り当て失敗 '{}'\n",
            Utils::Utf8ToWstring(name)));
        return E_FAIL;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = viewFormat; // SRGB/UNORM のビュー形式
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MostDetailedMip = 0;
    srv.Texture2D.MipLevels = mipCount;

    device->CreateShaderResourceView(
        texture.Get(),
        &srv,
        descAlloc.cpuHandle);

    // textures_ マップへの登録
    {
        std::lock_guard lock(mutex_);

        // 既に同じ key が存在している場合 → 古い SRV を deferred free する
        auto it = textures_.find(name);
        if (it != textures_.end()) {
            if (it->second && it->second->srvDesc.valid()) {
                DescriptorAllocator* alloc = dx12Mgr_->GetPersistentDescAlloc();
                if (alloc) {
                    uint32_t frameIndex = 0;
                    if (dx12Mgr_->GetFrameSync())
                        frameIndex = dx12Mgr_->GetFrameSync()->GetFrameIndex();

                    // GPU が使い終わったタイミングで安全に解放する
                    alloc->DeferFree(it->second->srvDesc, frameIndex);
                }
            }
        }

        // 新しいテクスチャデータを登録
        auto tex = std::make_unique<Texture>();
        tex->resource = texture;
        tex->srvDesc = descAlloc;
        tex->width = width;
        tex->height = height;
        tex->mipLevels = mipCount;
        tex->format = viewFormat;

        textures_[name] = std::move(tex);
    }

    return S_OK;
}

void TextureManager::UnloadAll()
{
    auto* allocator = dx12Mgr_->GetPersistentDescAlloc();
    if (allocator) {
        uint32_t frameIndex = dx12Mgr_->GetFrameSync()->GetFrameIndex();
        for (auto& [name, tex] : textures_) {
            if (tex && tex->srvDesc.valid()) {
                allocator->DeferFree(tex->srvDesc, frameIndex);
            }
        }
    }

    textures_.clear();
}
