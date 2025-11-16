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

    // パスとキーの正規化処理
    fs::path p(name);
    fs::path fullPath;
    std::string key;

    if (p.is_absolute()) {
        // name が絶対パスの場合そのまま使用
        fullPath = p;
        key = p.filename().string();
    }
    else if (p.has_parent_path()) {
        // rootが指定されているならrootを基準に結合する
        if (!root.empty()) {
            fullPath = fs::path(root) / p;
        }
        else {
            fullPath = p;
        }
        key = p.filename().string();
    }
    else {
        // root が指定されている場合は root と結合
        if (!root.empty()) {
            fullPath = fs::path(root) / p;
        }
        else {
            fullPath = p;
        }
        key = p.filename().string();
    }

    // すでに読み込み済みなら処理をスキップ
    if (TextureManager::GetInstance()->Has(key)) return S_OK;

    // 実際にファイルが存在しているかチェック
    if (!fs::exists(fullPath)) {
        Utils::Log(std::format(
            L"[TextureLoader] テクスチャファイルが見つかりません: {}\n",
            ToWString(fullPath.string())));
        return E_FAIL;
    }

    const std::string filepath = fullPath.string();

    // DirectXTex(WIC)による読み込みを最初に試す
    {
        ScratchImage srcImg, mipImg;
        std::wstring wpath = fullPath.wstring();
        auto flags = srgb ? WIC_FLAGS_FORCE_SRGB : WIC_FLAGS_NONE;

        HRESULT hr = LoadFromWICFile(wpath.c_str(), flags, nullptr, srcImg);
        if (SUCCEEDED(hr)) {

            // --- Mip生成 ---
            hr = GenerateMipMaps(
                srcImg.GetImages(),
                srcImg.GetImageCount(),
                srcImg.GetMetadata(),
                srgb ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT,
                0,
                mipImg);

            if (FAILED(hr)) {
                Utils::Log(std::format(
                    L"[TextureLoader] WICミップ生成に失敗しました (hr=0x{:08X}) '{}'\n",
                    static_cast<unsigned>(hr), ToWString(key)));
                // stb へフォールバック
            }
            else {
                // --- GPU アップロードは TextureManager に委譲 ---
                DXGI_FORMAT viewFmt =
                    srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                    : DXGI_FORMAT_R8G8B8A8_UNORM;

                return TextureManager::GetInstance()
                    ->CreateFromScratchImage(key, mipImg, viewFmt);
            }
        }
    }

    // DirectXTexで読み込めなかった場合→stb_imageにフォールバック
    int w = 0, h = 0, ch = 0;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &w, &h, &ch, 4);
    if (!pixels) {
        Utils::Log(std::format(
            L"[TextureLoader] stb_image による読み込みに失敗: '{}'\n",
            ToWString(filepath)));
        return E_FAIL;
    }

    // stb の生データから ScratchImage を構築し、
    // そこから mip を作成する
    ScratchImage src;
    HRESULT hr = src.Initialize2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        static_cast<size_t>(w),
        static_cast<size_t>(h),
        1, 1);
    if (FAILED(hr)) {
        stbi_image_free(pixels);
        Utils::Log(std::format(
            L"[TextureLoader] ScratchImage 初期化失敗 (hr=0x{:08X}) '{}'\n",
            static_cast<unsigned>(hr), ToWString(key)));
        return E_FAIL;
    }

    // stb の pixel データを ScratchImage にコピー
    const Image* img = src.GetImage(0, 0, 0);
    for (UINT y = 0; y < static_cast<UINT>(h); ++y) {
        memcpy(reinterpret_cast<uint8_t*>(img->pixels) + y * img->rowPitch,
            pixels + y * (w * 4),
            w * 4);
    }

    // mip 生成
    ScratchImage mipChain;
    hr = GenerateMipMaps(
        src.GetImages(),
        src.GetImageCount(),
        src.GetMetadata(),
        TEX_FILTER_DEFAULT,
        0,
        mipChain);

    // stb メモリ解放
    stbi_image_free(pixels);

    if (FAILED(hr)) {
        Utils::Log(std::format(
            L"[TextureLoader] stb経由のミップ生成に失敗 (hr=0x{:08X}) '{}'\n",
            static_cast<unsigned>(hr), ToWString(key)));

        // 単一レベルでフォールバック
        hr = mipChain.InitializeFromImage(*img);
        if (FAILED(hr)) {
            Utils::Log(std::format(
                L"[TextureLoader] 単一レベル生成にも失敗 (hr=0x{:08X}) '{}'\n",
                static_cast<unsigned>(hr), ToWString(key)));
            return E_FAIL;
        }
    }

    // Managerに登録
    DXGI_FORMAT viewFmt =
        srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
        : DXGI_FORMAT_R8G8B8A8_UNORM;

    return TextureManager::GetInstance()
        ->CreateFromScratchImage(key, mipChain, viewFmt);
}

HRESULT TextureLoader::LoadFromScene(const aiScene* scene, const std::string& root, bool srgb)
{
    if (!scene || !scene->HasMaterials()) return S_OK;

    HRESULT overall = S_OK;

    for (unsigned int mi = 0; mi < scene->mNumMaterials; ++mi) {
        const aiMaterial* mat = scene->mMaterials[mi];
        if (!mat) continue;

        HRESULT hr = LoadFromMaterial(mat, root, srgb);
        if (FAILED(hr)) overall = hr; // 失敗しても続行
    }

    return overall;
}

HRESULT TextureLoader::LoadFromMaterial(const aiMaterial* mat, const std::string& root, bool srgb)
{
    if (!mat) return E_FAIL;

    aiString texPath;
    if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0) return S_OK;

    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
        return E_FAIL;

    // 相対パスとして渡されたパスをそのまま Loader に渡す
    std::string rel = texPath.C_Str();

    return LoadFromFile(root, rel, srgb);
}
