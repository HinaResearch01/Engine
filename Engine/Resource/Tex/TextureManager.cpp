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

TextureManager::TextureManager()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

void TextureManager::Emplace(const std::string& name, std::unique_ptr<Texture> tex)
{
    if (!tex) return;
    textures_[name] = std::move(tex);
}

HRESULT TextureManager::CreateFromScratchImage(const std::string& name, const DirectX::ScratchImage& mipChain, DXGI_FORMAT viewFormat)
{
    if (mipChain.GetImageCount() == 0) {
        Utils::Log(std::format(L"[TextureManager] CreateFromScratchImage - empty image for '{}'\n", Utils::Utf8ToWstring(name)));
        return E_INVALIDARG;
    }

    if (!dx12Mgr_) {
        Utils::Log(L"[TextureManager] DX12Manager not available\n");
        return E_FAIL;
    }

    ID3D12Device* device = dx12Mgr_->GetDevice();
    CommandContext* cmdCtx = dx12Mgr_->GetCommandContext();
    DescriptorAllocator* allocator = dx12Mgr_->GetPersistentDescAlloc();
    if (!device || !cmdCtx || !allocator) {
        Utils::Log(L"[TextureManager] Missing DX subsystems in CreateFromScratchImage\n");
        return E_FAIL;
    }

    const TexMetadata& meta = mipChain.GetMetadata();
    UINT width = static_cast<UINT>(meta.width);
    UINT height = static_cast<UINT>(meta.height);
    UINT mipCount = static_cast<UINT>(meta.mipLevels);
    if (mipCount == 0) mipCount = 1;

    // Describe resource (use metadata.format as underlying resource format)
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = meta.width;
    desc.Height = static_cast<UINT>(meta.height);
    desc.DepthOrArraySize = static_cast<UINT16>(meta.arraySize);
    desc.MipLevels = static_cast<UINT16>(meta.mipLevels);
    desc.Format = meta.format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> texture;
    CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"[TextureManager] CreateCommittedResource(default) failed (hr=0x{:08X}) '{}'\n", static_cast<unsigned>(hr), Utils::Utf8ToWstring(name)));
        return hr;
    }

    // Prepare subresources
    std::vector<D3D12_SUBRESOURCE_DATA> subres;
    PrepareUpload(device, mipChain.GetImages(), mipChain.GetImageCount(), meta, subres);

    UINT64 uploadBytes = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<UINT>(subres.size()));
    ComPtr<ID3D12Resource> upload;
    CD3DX12_HEAP_PROPERTIES upHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);
    hr = device->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &upDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"[TextureManager] CreateCommittedResource(upload) failed (hr=0x{:08X}) '{}'\n", static_cast<unsigned>(hr), Utils::Utf8ToWstring(name)));
        return hr;
    }

    ID3D12GraphicsCommandList* list = cmdCtx->GetList();
    if (!list) {
        Utils::Log(L"[TextureManager] Command list null in CreateFromScratchImage\n");
        return E_FAIL;
    }

    UpdateSubresources(list, texture.Get(), upload.Get(), 0, 0, static_cast<UINT>(subres.size()), subres.data());
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    list->ResourceBarrier(1, &barrier);

    hr = cmdCtx->ExecuteAndWait();
    if (FAILED(hr)) {
        Utils::Log(std::format(L"[TextureManager] ExecuteAndWait failed (hr=0x{:08X}) '{}'\n", static_cast<unsigned>(hr), Utils::Utf8ToWstring(name)));
        return hr;
    }

    // Create SRV using viewFormat requested (e.g. SRGB view) but resource uses meta.format
    DescAlloc descAlloc = allocator->Allocate(1);
    if (!descAlloc.valid()) {
        Utils::Log(std::format(L"[TextureManager] Descriptor allocation failed for '{}'\n", Utils::Utf8ToWstring(name)));
        return E_FAIL;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = viewFormat;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MostDetailedMip = 0;
    srv.Texture2D.MipLevels = mipCount;
    srv.Texture2D.PlaneSlice = 0;
    srv.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(texture.Get(), &srv, descAlloc.cpuHandle);

    // store
    auto tex = std::make_unique<Texture>();
    tex->resource = texture;
    tex->srvDesc = descAlloc;
    tex->width = width;
    tex->height = height;
    tex->mipLevels = mipCount;
    tex->format = viewFormat;

    textures_.emplace(name, std::move(tex));
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
