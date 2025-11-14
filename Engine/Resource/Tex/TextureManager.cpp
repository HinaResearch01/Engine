#include "TextureManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/UtilsLog.h"
#include "stb_image.h"
#include <DirectXTex.h>

using namespace Tsumi::Resource;
using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

static std::wstring ToWString(const std::string& s) {
	return std::wstring(s.begin(), s.end());
}

TextureManager::TextureManager()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

bool TextureManager::Load(const std::string& root, const std::string& name, bool srgb)
{
    if (Has(name)) return true;

    // build path
    std::string filepath = root;
    if (!filepath.empty() && filepath.back() != '/' && filepath.back() != '\\')
        filepath += '/';
    filepath += name;

    //------------------------------------------------------------------
    // Try DirectXTex (WIC)
    //------------------------------------------------------------------
    {
        ScratchImage srcImg, mipImg;
        std::wstring wpath(filepath.begin(), filepath.end());
        auto flags = srgb ? WIC_FLAGS_FORCE_SRGB : WIC_FLAGS_NONE;

        HRESULT hr = LoadFromWICFile(wpath.c_str(), flags, nullptr, srcImg);
        if (SUCCEEDED(hr)) {
            hr = GenerateMipMaps(
                srcImg.GetImages(),
                srcImg.GetImageCount(),
                srcImg.GetMetadata(),
                srgb ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT,
                0, mipImg);

            if (SUCCEEDED(hr)) {
                const TexMetadata& meta = mipImg.GetMetadata();
                auto* dx = DX12Manager::GetInstance();
                ID3D12Device* device = dx->GetDevice();
                CommandContext* cmdCtx = dx->GetCommandContext();
                DescriptorAllocator* allocator = dx->GetPersistentDescAlloc();

                //--- create texture resource ---
                D3D12_RESOURCE_DESC desc{};
                desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(meta.dimension);
                desc.Width = static_cast<UINT>(meta.width);
                desc.Height = static_cast<UINT>(meta.height);
                desc.DepthOrArraySize = static_cast<UINT16>(meta.arraySize);
                desc.MipLevels = static_cast<UINT16>(meta.mipLevels);
                desc.Format = srgb ? MakeSRGB(meta.format) : meta.format;
                desc.SampleDesc.Count = 1;
                desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

                ComPtr<ID3D12Resource> texture;
                CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
                hr = device->CreateCommittedResource(
                    &heap, D3D12_HEAP_FLAG_NONE, &desc,
                    D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                    IID_PPV_ARGS(&texture));
                if (FAILED(hr)) {
                    Utils::Log(std::format(L"[TextureManager] CreateCommittedResource failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
                    return false;
                }

                //--- upload mip data ---
                std::vector<D3D12_SUBRESOURCE_DATA> subres;
                PrepareUpload(device, mipImg.GetImages(), mipImg.GetImageCount(), meta, subres);

                UINT64 uploadBytes = GetRequiredIntermediateSize(texture.Get(), 0, (UINT)subres.size());
                ComPtr<ID3D12Resource> upload;
                CD3DX12_HEAP_PROPERTIES upHeap(D3D12_HEAP_TYPE_UPLOAD);
                auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);
                hr = device->CreateCommittedResource(
                    &upHeap, D3D12_HEAP_FLAG_NONE, &upDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                    IID_PPV_ARGS(&upload));
                if (FAILED(hr)) {
                    Utils::Log(std::format(L"[TextureManager] CreateCommittedResource failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
                    return false;
                }


                ID3D12GraphicsCommandList* list = cmdCtx->GetList();
                UpdateSubresources(list, texture.Get(), upload.Get(), 0, 0, (UINT)subres.size(), subres.data());
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    texture.Get(),
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                list->ResourceBarrier(1, &barrier);
                cmdCtx->ExecuteAndWait();

                //--- create SRV ---
                auto descAlloc = allocator->Allocate(1);
                if (!descAlloc.valid()) {
                    Utils::Log(std::format(L"[TextureManager] Descriptor allocation failed for '{}'\n", ToWString(name)));
                    return false;
                }
                D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
                srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srv.Format = desc.Format;
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srv.Texture2D.MostDetailedMip = 0;
                srv.Texture2D.MipLevels = (UINT)meta.mipLevels;

                device->CreateShaderResourceView(texture.Get(), &srv, descAlloc.cpuHandle);

                //--- store ---
                auto tex = std::make_unique<Texture>();
                tex->resource = texture;
                tex->srvDesc = descAlloc;
                tex->width = (UINT)meta.width;
                tex->height = (UINT)meta.height;
                tex->mipLevels = (UINT)meta.mipLevels;
                tex->format = desc.Format;

                textures_.emplace(name, std::move(tex));
                return true;
            }
        }
    }

    //------------------------------------------------------------------
    // Fallback: stb_image
    //------------------------------------------------------------------
    int w = 0, h = 0, ch = 0;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &w, &h, &ch, 4);
    if (!pixels) {
        Utils::Log(std::format(L"[TextureManager] Failed to load '{}'\n", ToWString(filepath)));
        return false;
    }

    DXGI_FORMAT fmt = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    Texture* t = CreateTextureFromMemory(name, pixels, w, h, fmt, 4);
    stbi_image_free(pixels);
    return (t != nullptr);
}

Texture* TextureManager::CreateTextureFromMemory(
	const std::string& name, const uint8_t* pixels,
	UINT width, UINT height, DXGI_FORMAT format, UINT bytesPerPixel)
{
	auto* dx = DX12Manager::GetInstance();
	ID3D12Device* device = dx->GetDevice();
	CommandContext* cmdCtx = dx->GetCommandContext();
	DescriptorAllocator* allocator = dx->GetPersistentDescAlloc();

	//--- generate mip chain ---
	ScratchImage src, mip;
	src.Initialize2D(format, width, height, 1, 1);
	const Image* img = src.GetImage(0, 0, 0);
	for (UINT y = 0; y < height; ++y)
		memcpy(img->pixels + y * img->rowPitch, pixels + y * width * bytesPerPixel, width * bytesPerPixel);

	GenerateMipMaps(src.GetImages(), src.GetImageCount(), src.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mip);

	const TexMetadata& meta = mip.GetMetadata();

	//--- create GPU resource ---
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = (UINT16)meta.mipLevels;
	desc.Format = format;
	desc.SampleDesc.Count = 1;

	ComPtr<ID3D12Resource> texture;
	CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device->CreateCommittedResource(
		&heap, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&texture));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"[TextureManager] CreateCommittedResource failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return nullptr;
    }

	//--- upload data ---
	std::vector<D3D12_SUBRESOURCE_DATA> subres;
	PrepareUpload(device, mip.GetImages(), mip.GetImageCount(), meta, subres);

	UINT64 uploadBytes = GetRequiredIntermediateSize(texture.Get(), 0, (UINT)subres.size());
	ComPtr<ID3D12Resource> upload;
	CD3DX12_HEAP_PROPERTIES upHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);
    hr = device->CreateCommittedResource(
		&upHeap, D3D12_HEAP_FLAG_NONE, &upDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&upload));
    if (FAILED(hr)) {
        Utils::Log(std::format(L"[TextureManager] CreateCommittedResource failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return nullptr;
    }


	ID3D12GraphicsCommandList* list = cmdCtx->GetList();
	UpdateSubresources(list, texture.Get(), upload.Get(), 0, 0, (UINT)subres.size(), subres.data());
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	list->ResourceBarrier(1, &barrier);
	cmdCtx->ExecuteAndWait();

	//--- create SRV ---
	auto descAlloc = allocator->Allocate(1);
	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Format = format;
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Texture2D.MostDetailedMip = 0;
	srv.Texture2D.MipLevels = (UINT)meta.mipLevels;

	device->CreateShaderResourceView(texture.Get(), &srv, descAlloc.cpuHandle);

	//--- store ---
	auto tex = std::make_unique<Texture>();
	tex->resource = texture;
	tex->srvDesc = descAlloc;
	tex->width = width;
	tex->height = height;
	tex->mipLevels = (UINT)meta.mipLevels;
	tex->format = format;

	Texture* ret = tex.get();
	textures_.emplace(name, std::move(tex));
	return ret;
}

void TextureManager::UnloadAll()
{
    auto* allocator = dx12Mgr_->GetPersistentDescAlloc();
    if (allocator) {
        for (auto& [name, tex] : textures_) {
            if (tex && tex->srvDesc.valid()) {
                allocator->Free(tex->srvDesc);
            }
        }
    }

    textures_.clear();
}
