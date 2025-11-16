#include "TextureLoader.h"
#include "Resource/Tex/TextureManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/UtilsLog.h"
#include <assimp/scene.h>
#include <assimp/material.h>
#include "stb_image.h"
#include <DirectXTex.h>
#include <filesystem>
#include <format>

using namespace Tsumi::Loader;
using namespace Tsumi::Resource;
using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;
using namespace DirectX;
namespace fs = std::filesystem;

static std::wstring ToWString(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

HRESULT TextureLoader::LoadFromFile(const std::string& root, const std::string& name, bool srgb)
{
    if (name.empty()) return E_INVALIDARG;
    if (TextureManager::GetInstance()->Has(name)) return S_OK;

    // build filepath
    std::string filepath = root;
    if (!filepath.empty() && filepath.back() != '/' && filepath.back() != '\\') filepath += '/';
    filepath += name;

    // Try WIC via DirectXTex first
    {
        ScratchImage srcImg, mipImg;
        std::wstring wpath = fs::path(filepath).wstring();
        auto flags = srgb ? WIC_FLAGS_FORCE_SRGB : WIC_FLAGS_NONE;

        HRESULT hr = LoadFromWICFile(wpath.c_str(), flags, nullptr, srcImg);
        if (SUCCEEDED(hr)) {
            // generate mip chain (CPU)
            hr = GenerateMipMaps(srcImg.GetImages(), srcImg.GetImageCount(), srcImg.GetMetadata(), srgb ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT, 0, mipImg);
            if (FAILED(hr)) {
                Utils::Log(std::format(L"[TextureLoader] GenerateMipMaps failed (hr=0x{:08X}) '{}'\n", static_cast<unsigned>(hr), ToWString(name)));
                // fallthrough to stb fallback
            }
            else {
                // delegate upload/management to TextureManager
                DXGI_FORMAT viewFmt = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
                return TextureManager::GetInstance()->CreateFromScratchImage(name, mipImg, viewFmt);
            }
        }
    }

    // Fallback: stb_image
    int w = 0, h = 0, ch = 0;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &w, &h, &ch, 4);
    if (!pixels) {
        Utils::Log(std::format(L"[TextureLoader] Failed to load (stb_image) '{}'\n", ToWString(filepath)));
        return E_FAIL;
    }

    // create ScratchImage from raw pixels and generate mips
    ScratchImage src;
    HRESULT hr = src.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, w, h, 1, 1);
    if (FAILED(hr)) {
        stbi_image_free(pixels);
        Utils::Log(std::format(L"[TextureLoader] ScratchImage Initialize2D failed (hr=0x{:08X}) '{}'\n", static_cast<unsigned>(hr), ToWString(name)));
        return E_FAIL;
    }
    const Image* img = src.GetImage(0, 0, 0);
    for (UINT y = 0; y < (UINT)h; ++y) {
        memcpy(reinterpret_cast<uint8_t*>(img->pixels) + y * img->rowPitch,
            pixels + y * (w * 4),
            w * 4);
    }
    ScratchImage mipChain;
    hr = GenerateMipMaps(src.GetImages(), src.GetImageCount(), src.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain);
    stbi_image_free(pixels);
    if (FAILED(hr)) {
        Utils::Log(std::format(L"[TextureLoader] GenerateMipMaps (stb path) failed (hr=0x{:08X}) '{}'\n", static_cast<unsigned>(hr), ToWString(name)));
        // try to upload single-level src as fallback
        hr = mipChain.InitializeFromImage(*img);
        if (FAILED(hr)) {
            Utils::Log(std::format(L"[TextureLoader] InitializeFromImage fallback failed (hr=0x{:08X}) '{}'\n", static_cast<unsigned>(hr), ToWString(name)));
            return E_FAIL;
        }
    }

    DXGI_FORMAT viewFmt = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    return TextureManager::GetInstance()->CreateFromScratchImage(name, mipChain, viewFmt);
}

HRESULT TextureLoader::LoadFromScene(const aiScene* scene, const std::string& root, bool srgb)
{
    if (!scene || !scene->HasMaterials()) return S_OK;

    HRESULT overall = S_OK;
    for (unsigned int mi = 0; mi < scene->mNumMaterials; ++mi) {
        const aiMaterial* mat = scene->mMaterials[mi];
        if (!mat) continue;
        HRESULT hr = LoadFromMaterial(mat, root, srgb);
        if (FAILED(hr)) overall = hr; // record failure but continue
    }
    return overall;
}

HRESULT TextureLoader::LoadFromMaterial(const aiMaterial* mat, const std::string& root, bool srgb)
{
    if (!mat) return E_FAIL;
    aiString texPath;
    if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0) return S_OK; // nothing to do
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS) return E_FAIL;

    std::string rel = texPath.C_Str();
    return LoadFromFile(root, rel, srgb);
}
