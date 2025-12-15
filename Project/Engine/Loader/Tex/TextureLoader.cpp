#include "TextureLoader.h"
#include "Resource/Tex/TextureManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
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

// root + name から管理用キーを生成する
static std::string MakeKeyFromRoot(const std::string& root, const std::string& name)
{
	fs::path p(name);
	fs::path full;
	if (p.is_absolute()) {
		// 絶対パスの場合、可能であれば root からの相対パスに変換
		// 失敗した場合は絶対パスをそのままキーとして使用
		if (!root.empty()) {
			try {
				fs::path rootp = fs::path(root);
				fs::path rel = fs::relative(p, rootp);
				if (!rel.empty()) {
					return (rootp / rel).lexically_normal().string();
				}
			}
			catch (...) {
				// 相対化に失敗した場合は絶対パスを使用
			}
		}
		return p.lexically_normal().string();
	}
	else {
		// 相対パスの場合は root を基準にフルパス化
		if (!root.empty()) full = fs::path(root) / p;
		else full = p;
		return full.lexically_normal().string();
	}
}

HRESULT TextureLoader::Load(const std::string& fullPath, const std::string& alias, bool srgb = false)
{
	auto* texMgr = TextureManager::GetInstance();

	std::string key = MakeKeyFromRoot("", fullPath);

	if (texMgr->HasAlias(alias))
		return S_OK;

	// 1. CPUデコード
	ScratchImage mipChain;
	HRESULT hr{};
	HRESULT hr = DecodeToScratchImage(fullPath, srgb, mipChain);
	if (FAILED(hr)) return hr;

	// 2. Manager に登録依頼
	hr = texMgr->RegisterTexture(
		key,
		mipChain,
		srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		: DXGI_FORMAT_R8G8B8A8_UNORM);
	if (FAILED(hr)) return hr;

	// 3. alias 登録
	texMgr->RegisterAlias(alias, key);

	return S_OK;
}

HRESULT Tsumi::Loader::TextureLoader::ExtractSceneTextures(const aiScene* scene, const std::string& fullPath, const std::string& alias, bool srgb)
{
	if (!scene || !scene->HasMaterials())
		return S_OK;

	// モデルファイルのあるディレクトリ
	std::filesystem::path modelPath(fullPath);
	std::filesystem::path baseDir = modelPath.parent_path();

	HRESULT overall = S_OK;

	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial* mat = scene->mMaterials[i];
		if (!mat) continue;

		// Diffuse テクスチャ（まずは1枚目だけ）
		aiString texPath;
		if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0)
			continue;

		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
			continue;

		// Assimp が返すのは「相対パス」であることが多い
		std::filesystem::path rel(texPath.C_Str());
		std::filesystem::path fullTexPath = baseDir / rel;

		// alias を組み立てる（衝突回避）
		std::string alias =
			alias + "_diffuse_" + std::to_string(i);

		// 普通の Load に委譲
		HRESULT hr = Load(
			fullTexPath.string(),
			alias,
			srgb);

		if (FAILED(hr))
			overall = hr;
	}

	return overall;
}

HRESULT TextureLoader::DecodeToScratchImage(const std::string& path, bool srgb, DirectX::ScratchImage& outImage)
{
	using namespace DirectX;

	outImage.Release();

	std::filesystem::path p(path);
	if (!std::filesystem::exists(p))
		return E_FAIL;

	const std::wstring wpath = p.wstring();
	const auto ext = p.extension().wstring();

	HRESULT hr = S_OK;

	// =========================
	// DDS（ミップ・圧縮そのまま）
	// =========================
	if (_wcsicmp(ext.c_str(), L".dds") == 0)
	{
		hr = LoadFromDDSFile(
			wpath.c_str(),
			DDS_FLAGS_NONE,
			nullptr,
			outImage);

		return hr;
	}

	// =========================
	// WIC（png / jpg / bmp 等）
	// =========================
	{
		ScratchImage src;
		WIC_FLAGS flags = srgb
			? WIC_FLAGS_FORCE_SRGB
			: WIC_FLAGS_NONE;

		hr = LoadFromWICFile(
			wpath.c_str(),
			flags,
			nullptr,
			src);

		if (SUCCEEDED(hr))
		{
			// ミップマップ生成
			hr = GenerateMipMaps(
				src.GetImages(),
				src.GetImageCount(),
				src.GetMetadata(),
				srgb ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT,
				0,
				outImage);

			// ミップ生成失敗時は元画像だけ使う
			if (FAILED(hr))
			{
				outImage = std::move(src);
				return S_OK;
			}

			return S_OK;
		}
	}

	// =========================
	// stb_image フォールバック
	// =========================
	int w = 0, h = 0, ch = 0;
	stbi_uc* pixels = stbi_load(
		path.c_str(),
		&w,
		&h,
		&ch,
		4);

	if (!pixels)
		return E_FAIL;

	ScratchImage src;
	hr = src.Initialize2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		static_cast<size_t>(w),
		static_cast<size_t>(h),
		1,
		1);

	if (FAILED(hr))
	{
		stbi_image_free(pixels);
		return hr;
	}

	const Image* img = src.GetImage(0, 0, 0);
	for (int y = 0; y < h; ++y)
	{
		memcpy(
			reinterpret_cast<uint8_t*>(img->pixels) + y * img->rowPitch,
			pixels + y * (w * 4),
			w * 4);
	}

	stbi_image_free(pixels);

	// ミップ生成
	hr = GenerateMipMaps(
		src.GetImages(),
		src.GetImageCount(),
		src.GetMetadata(),
		TEX_FILTER_DEFAULT,
		0,
		outImage);

	if (FAILED(hr))
	{
		outImage = std::move(src);
		return S_OK;
	}

	return S_OK;
}
