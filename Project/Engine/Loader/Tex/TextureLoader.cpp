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

HRESULT TextureLoader::LoadFromFile(const std::string& root, const std::string& name, bool srgb)
{
	if (name.empty()) return E_INVALIDARG;

	// root + 相対パスのルールでフルパスとキーを構築
	fs::path p(name);
	fs::path fullPath;
	if (p.is_absolute()) {
		fullPath = p;
	}
	else {
		if (!root.empty()) fullPath = fs::path(root) / p;
		else fullPath = p;
	}
	std::string key = MakeKeyFromRoot(root, name);

	// すでに読み込まれていれば何もしない
	if (TextureManager::GetInstance()->Has(key)) return S_OK;

	// ファイル存在チェック
	if (!fs::exists(fullPath)) {
		Utils::Log(std::format(
			L"[TextureLoader] テクスチャファイルが見つかりません: {}\n",
			Utils::Utf8ToWstring(fullPath.string())));
		return E_FAIL;
	}

	const std::string filepath = fullPath.string();

	// まず DirectXTex（WIC）での読み込みを試みる
	{
		ScratchImage srcImg, mipImg;
		std::wstring wpath = fullPath.wstring();
		auto flags = srgb ? WIC_FLAGS_FORCE_SRGB : WIC_FLAGS_NONE;

		HRESULT hr = LoadFromWICFile(wpath.c_str(), flags, nullptr, srcImg);
		if (SUCCEEDED(hr)) {
			// ミップマップ生成
			hr = GenerateMipMaps(
				srcImg.GetImages(),
				srcImg.GetImageCount(),
				srcImg.GetMetadata(),
				srgb ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT,
				0,
				mipImg);

			if (FAILED(hr)) {
				Utils::Log(std::format(
					L"[TextureLoader] GenerateMipMaps 失敗 (hr=0x{:08X}) '{}'\n",
					static_cast<unsigned>(hr),
					Utils::Utf8ToWstring(key)));
				// stb_image にフォールバック
			}
			else {
				// sRGB / Linear に応じたビュー用フォーマット
				auto viewFmt = srgb
					? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
					: DXGI_FORMAT_R8G8B8A8_UNORM;

				return TextureManager::GetInstance()->CreateFromScratchImage(
					key, mipImg, viewFmt);
			}
		}
	}

	// stb_image を使ったフォールバック読み込み
	int w = 0, h = 0, ch = 0;
	stbi_uc* pixels = stbi_load(filepath.c_str(), &w, &h, &ch, 4);
	if (!pixels) {
		Utils::Log(std::format(
			L"[TextureLoader] 読み込み失敗 (stb_image) '{}'\n",
			Utils::Utf8ToWstring(filepath)));
		return E_FAIL;
	}

	// 生ピクセルから ScratchImage を作成し、ミップマップを生成
	ScratchImage src;
	HRESULT hr = src.Initialize2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		static_cast<size_t>(w),
		static_cast<size_t>(h),
		1,
		1);

	if (FAILED(hr)) {
		stbi_image_free(pixels);
		Utils::Log(std::format(
			L"[TextureLoader] ScratchImage Initialize2D 失敗 (hr=0x{:08X}) '{}'\n",
			static_cast<unsigned>(hr),
			Utils::Utf8ToWstring(key)));
		return E_FAIL;
	}

	// ピクセルデータを ScratchImage にコピー
	const Image* img = src.GetImage(0, 0, 0);
	for (UINT y = 0; y < static_cast<UINT>(h); ++y) {
		memcpy(
			reinterpret_cast<uint8_t*>(img->pixels) + y * img->rowPitch,
			pixels + y * (w * 4),
			w * 4);
	}

	ScratchImage mipChain;
	hr = GenerateMipMaps(
		src.GetImages(),
		src.GetImageCount(),
		src.GetMetadata(),
		TEX_FILTER_DEFAULT,
		0,
		mipChain);

	stbi_image_free(pixels);

	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"[TextureLoader] GenerateMipMaps (stb 経路) 失敗 (hr=0x{:08X}) '{}'\n",
			static_cast<unsigned>(hr),
			Utils::Utf8ToWstring(key)));

		// フォールバック：ミップなしで単一レベルを使用
		hr = mipChain.InitializeFromImage(*img);
		if (FAILED(hr)) {
			Utils::Log(std::format(
				L"[TextureLoader] InitializeFromImage フォールバック失敗 (hr=0x{:08X}) '{}'\n",
				static_cast<unsigned>(hr),
				Utils::Utf8ToWstring(key)));
			return E_FAIL;
		}
	}

	auto viewFmt = srgb
		? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		: DXGI_FORMAT_R8G8B8A8_UNORM;

	return TextureManager::GetInstance()->CreateFromScratchImage(
		key, mipChain, viewFmt);
}

HRESULT TextureLoader::LoadFromScene(const aiScene* scene, const std::string& root, bool srgb)
{
	if (!scene || !scene->HasMaterials()) return S_OK;

	HRESULT overall = S_OK;
	for (unsigned int mi = 0; mi < scene->mNumMaterials; ++mi) {
		const aiMaterial* mat = scene->mMaterials[mi];
		if (!mat) continue;

		// マテリアルからテクスチャをロード
		HRESULT hr = LoadFromMaterial(mat, root, srgb);
		if (FAILED(hr)) overall = hr;
	}
	return overall;
}

HRESULT TextureLoader::LoadFromMaterial(const aiMaterial* mat, const std::string& root, bool srgb)
{
	if (!mat) return E_FAIL;

	aiString texPath;

	// Diffuse テクスチャがなければ何もしない
	if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0) return S_OK;

	// 最初の Diffuse テクスチャパスを取得
	if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
		return E_FAIL;

	std::string rel = texPath.C_Str();
	return LoadFromFile(root, rel, srgb);
}
