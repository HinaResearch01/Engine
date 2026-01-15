#include "TextureLoader.h"
#include "Resource/ResourceSystem.h" // 追加
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/Logger.h"
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

HRESULT TextureLoader::Load(const std::string& fullPath, const std::string& alias, bool srgb)
{
	// パスが空の場合は無効
	if (fullPath.empty())
		return E_INVALIDARG;

	// 実キー（正規化されたパス）を生成
	const std::string key =
		Utils::Func::MakeKeyFromRoot("", fullPath);

	// すでに同じ実キーが登録済みの場合は
	// alias の紐づけだけ行って早期リターン
	if (TryResolveAlias(key, alias))
		return S_OK;

	// ファイル存在チェック
	if (!fs::exists(fullPath))
		return E_FAIL;

	// CPU 側で画像をデコード（DDS / WIC / stb のいずれか）
	ScratchImage image;
	HRESULT hr = DecodeToScratchImage(fullPath, srgb, image);
	if (FAILED(hr))
		return hr;

	// GPU リソースの生成と Manager への登録は共通処理に委譲
	return RegisterFromImage(key, image, alias, srgb);
}

HRESULT TextureLoader::LoadFromScene(const aiScene* scene, const std::string& modelPath, const std::string& alias, bool srgb)
{
	// マテリアルが無い場合は正常終了（テクスチャなしモデル）
	if (!scene || !scene->HasMaterials())
		return S_OK;

	// モデルファイルの存在するディレクトリ
	fs::path baseDir = fs::path(modelPath).parent_path();

	HRESULT overall = S_OK;

	// シーン内の全マテリアルを走査
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial* mat = scene->mMaterials[i];
		if (!mat) continue;

		// Diffuse テクスチャが無いマテリアルはスキップ
		if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0)
			continue;

		aiString texPath;
		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
			continue;

		// Assimp の返すパスは相対パスであることが多い
		fs::path fullTexPath = baseDir / texPath.C_Str();

		// マテリアルごとに alias を組み立てる（衝突回避）
		std::string buildAlias =
			alias + "_diffuse_" + std::to_string(i);

		// 通常の Load に処理を委譲
		HRESULT hr = Load(
			fullTexPath.string(),
			buildAlias,
			srgb);

		// ★追加: 最初のテクスチャ、あるいは「明示的に同じ名前で呼びたい」場合のために
		// ベース名(alias)単体でもアクセスできるように登録しておく。
		// ただし、複数マテリアルがある場合は上書きされるので「最後の1つ」か「最初の1つ」になる。
		// ここでは「最初の1つ」を優先する（あるいはユーザー運用でカバー）
		if (SUCCEEDED(hr)) {
			// まだ登録されてなければベース名も登録
			if (!ResourceSystem::GetInstance()->GetTextureManager()->HasAlias(alias)) {
				// 実キーを取得してエイリアス登録
				std::string key = Utils::Func::MakeKeyFromRoot("", fullTexPath.string());
				ResourceSystem::GetInstance()->GetTextureManager()->RegisterAlias(alias, key);
			}
		}

		// 失敗は記録するが、他のマテリアルは処理を続行
		if (FAILED(hr))
			overall = hr;
	}

	return overall;
}

HRESULT TextureLoader::RegisterFromImage(const std::string& key, const DirectX::ScratchImage& image, const std::string& alias, bool srgb)
{
	auto* texMgr = ResourceSystem::GetInstance()->GetTextureManager();

	// GPU リソースの生成と登録
	HRESULT hr = texMgr->RegisterTexture(
		key,
		image,
		srgb
		? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		: DXGI_FORMAT_R8G8B8A8_UNORM);

	if (FAILED(hr))
		return hr;

	// alias → key の対応付け
	texMgr->RegisterAlias(alias, key);
	return S_OK;
}

HRESULT TextureLoader::DecodeToScratchImage(const std::string& path, bool srgb, DirectX::ScratchImage& outImage)
{
	using namespace DirectX;

	// 既存データを解放
	outImage.Release();

	std::filesystem::path p(path);
	if (!std::filesystem::exists(p))
		return E_FAIL;

	const std::wstring wpath = p.wstring();
	const auto ext = p.extension().wstring();

	HRESULT hr = S_OK;

	// =========================
	// DDS（圧縮・ミップを保持）
	// =========================
	if (_wcsicmp(ext.c_str(), L".dds") == 0)
	{
		return LoadFromDDSFile(
			wpath.c_str(),
			DDS_FLAGS_NONE,
			nullptr,
			outImage);
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

			// ミップ生成に失敗した場合は元画像のみ使用
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
			static_cast<size_t>(w) * 4);
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
		// ミップ無しでフォールバック
		outImage = std::move(src);
		return S_OK;
	}

	return S_OK;
}

bool TextureLoader::TryResolveAlias(const std::string& key, const std::string& alias)
{
	auto* texMgr = ResourceSystem::GetInstance()->GetTextureManager();

	if (texMgr->HasKey(key)) {
		texMgr->RegisterAlias(alias, key);
		return true; // すでに登録済み
	}
	return false;
}
