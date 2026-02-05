#include "TextureLoader.h"

#include "Resource/ResourceSystem.h"
#include "Resource/Tex/TextureManager.h"
#include "Utils/Func/UtilFunc.h"
#include "Utils/Logger/Logger.h"

#include <assimp/scene.h>
#include <assimp/material.h>

#include <DirectXTex.h>
#include "stb_image.h"

#include <filesystem>
#include <format>

using namespace Tsumi::Loader;
using namespace Tsumi::Resource;
using namespace DirectX;
namespace fs = std::filesystem;

static std::string MakeKey(const std::string& fullPath)
{
	// 他と同じルールに統一（Mesh/Model と一致させる）
	return Tsumi::Utils::Func::MakeKeyFromRoot("", fullPath);
}

static std::string MakeMaterialDiffuseAlias(const std::string& baseAlias, unsigned matIndex)
{
	// 規則的で追いやすい alias
	return std::format("{}_mat{}_diffuse", baseAlias, matIndex);
}

static std::string MakeDefaultDiffuseAlias(const std::string& baseAlias)
{
	// Mesh 側が参照する代表テクスチャ alias
	return std::format("{}_diffuse", baseAlias);
}

HRESULT TextureLoader::Load(const std::string& fullPath, const std::string& alias, bool srgb)
{
	if (fullPath.empty())
		return E_INVALIDARG;

	// 実キー（正規化パス）
	const std::string key = MakeKey(fullPath);

	// すでに同じ実キーが登録済みなら alias の紐づけだけ
	if (TryResolveAlias(key, alias))
		return S_OK;

	// ファイル存在チェック
	if (!fs::exists(fullPath))
		return E_FAIL;

	// CPU decode
	ScratchImage image;
	HRESULT hr = DecodeToScratchImage(fullPath, srgb, image);
	if (FAILED(hr))
		return hr;

	// GPU 登録 + alias
	return RegisterFromImage(key, image, alias, srgb);
}

HRESULT TextureLoader::LoadFromScene(
	const aiScene* scene,
	const std::string& modelPath,
	const std::string& alias,
	bool srgb)
{
	if (!scene || !scene->HasMaterials())
		return S_OK;

	fs::path baseDir = fs::path(modelPath).parent_path();

	auto* texMgr = ResourceSystem::GetInstance()->GetTextureManager();
	if (!texMgr)
		return E_POINTER;

	HRESULT overall = S_OK;

	// 代表（default）テクスチャは「最初に見つかった diffuse」を採用する
	bool defaultBound = false;
	const std::string defaultAlias = MakeDefaultDiffuseAlias(alias);

	for (unsigned i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial* mat = scene->mMaterials[i];
		if (!mat) continue;

		if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0)
			continue;

		aiString texPath;
		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
			continue;

		// 相対パスが多い
		fs::path fullTexPath = baseDir / texPath.C_Str();

		// 正規化キー
		const std::string texKey = MakeKey(fullTexPath.string());

		// 実体が無いなら decode/register
		if (!texMgr->HasKey(texKey))
		{
			if (!fs::exists(fullTexPath)) {
				overall = E_FAIL;
				continue;
			}

			ScratchImage image;
			HRESULT hr = DecodeToScratchImage(fullTexPath.string(), srgb, image);
			if (FAILED(hr)) { overall = hr; continue; }

			const std::string materialAlias = MakeMaterialDiffuseAlias(alias, i);

			hr = RegisterFromImage(texKey, image, materialAlias, srgb);
			if (FAILED(hr)) { overall = hr; continue; }
		}
		else
		{
			// 実体は既にある。materialAlias を張るだけ
			const std::string materialAlias = MakeMaterialDiffuseAlias(alias, i);
			texMgr->RegisterAlias(materialAlias, texKey);
		}

		// 代表（default）alias を最初の diffuse に紐づける
		if (!defaultBound)
		{
			// 衝突しないよう、既存なら何もしない
			if (!texMgr->HasAlias(defaultAlias))
				texMgr->RegisterAlias(defaultAlias, texKey);
			defaultBound = true;
		}
	}

	return overall;
}

HRESULT TextureLoader::RegisterFromImage(
	const std::string& key,
	const DirectX::ScratchImage& image,
	const std::string& alias,
	bool srgb)
{
	auto* texMgr = ResourceSystem::GetInstance()->GetTextureManager();
	if (!texMgr) return E_POINTER;

	// GPU リソース生成と登録
	HRESULT hr = texMgr->RegisterTexture(
		key,
		image,
		srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		: DXGI_FORMAT_R8G8B8A8_UNORM);

	if (FAILED(hr))
		return hr;

	// alias → key
	texMgr->RegisterAlias(alias, key);
	return S_OK;
}

HRESULT TextureLoader::DecodeToScratchImage(
	const std::string& path,
	bool srgb,
	DirectX::ScratchImage& outImage)
{
	using namespace DirectX;

	outImage.Release();

	fs::path p(path);
	if (!fs::exists(p))
		return E_FAIL;

	const std::wstring wpath = p.wstring();
	const auto ext = p.extension().wstring();

	// DDS
	if (_wcsicmp(ext.c_str(), L".dds") == 0)
	{
		return LoadFromDDSFile(
			wpath.c_str(),
			DDS_FLAGS_NONE,
			nullptr,
			outImage);
	}

	// WIC
	{
		ScratchImage src;
		WIC_FLAGS flags = srgb ? WIC_FLAGS_FORCE_SRGB : WIC_FLAGS_NONE;

		HRESULT hr = LoadFromWICFile(wpath.c_str(), flags, nullptr, src);
		if (SUCCEEDED(hr))
		{
			hr = GenerateMipMaps(
				src.GetImages(),
				src.GetImageCount(),
				src.GetMetadata(),
				srgb ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT,
				0,
				outImage);

			if (FAILED(hr)) {
				outImage = std::move(src);
				return S_OK;
			}
			return S_OK;
		}
	}

	// stb fallback
	int w = 0, h = 0, ch = 0;
	stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &ch, 4);
	if (!pixels)
		return E_FAIL;

	ScratchImage src;
	HRESULT hr = src.Initialize2D(
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
			static_cast<size_t>(w) * 4);
	}
	stbi_image_free(pixels);

	// mip生成
	hr = GenerateMipMaps(
		src.GetImages(),
		src.GetImageCount(),
		src.GetMetadata(),
		TEX_FILTER_DEFAULT,
		0,
		outImage);

	if (FAILED(hr)) {
		outImage = std::move(src);
		return S_OK;
	}

	return S_OK;
}

bool TextureLoader::TryResolveAlias(const std::string& key, const std::string& alias)
{
	auto* texMgr = ResourceSystem::GetInstance()->GetTextureManager();
	if (!texMgr) return false;

	// 既に実体があるなら alias を張って終わり
	if (texMgr->HasKey(key)) {
		texMgr->RegisterAlias(alias, key);
		return true;
	}
	return false;
}
