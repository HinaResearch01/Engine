#pragma once

#include <string>
#include <stdexcept>
#include <format>
#include <wrl.h>
#include <d3d12.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <format>

#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include "Resource/Tex/TextureManager.h"
#include  "Loader/Tex/TextureLoader.h"
#include "Resource/Mesh/MeshManager.h"
#include "Loader/Mesh/MeshLoader.h"

struct mdl { std::string name; };
struct tex { std::string name; };
struct snd { std::string name; };

namespace Tsumi {

/* 各種リソースの共通インターフェース */
class ResourceAPI {

public:
	/// <summary>
	/// 読み込み処理
	/// </summary>
	template <typename T>
	static HRESULT Load(const std::string& root, const std::string& name);

	/// <summary>
	/// 所持確認
	/// </summary>
	template <typename T>
	static bool Has(const std::string& name);

	/// <summary>
	/// シーンリセット処理
	/// </summary>
	static HRESULT SceneReset();
};


template <>
inline HRESULT ResourceAPI::Load<mdl>(const std::string& root, const std::string& name)
{
	// ファイルパスを構築
	std::filesystem::path p(name);
	std::filesystem::path fullPath;
	if (p.is_absolute()) fullPath = p;
	else if (!root.empty()) fullPath = std::filesystem::path(root) / p;
	else fullPath = p;

	// ファイル存在チェック
	if (!std::filesystem::exists(fullPath)) {
		Utils::Log(std::format(
			L"[MeshLoader] ファイルが見つかりません: {}\n",
			Utils::Utf8ToWstring(fullPath.string())));
		return E_FAIL;
	}

	Assimp::Importer importer;
	const unsigned int flags =
		aiProcess_Triangulate |          // 面を三角形化
		aiProcess_FlipUVs |               // UV の上下反転
		aiProcess_FlipWindingOrder |      // 面の向きを反転
		aiProcess_CalcTangentSpace;       // 接線・従法線計算

	// Assimpでモデル読み込み
	const aiScene* scene = importer.ReadFile(fullPath.string().c_str(), flags);
	if (!scene) {
		Utils::Log(std::format(
			L"[MeshLoader] Assimp 読み込み失敗: {}\n",
			Utils::Utf8ToWstring(importer.GetErrorString())));
		return E_FAIL;
	}

	// シーンの解析とメッシュ生成は LoadFromScene に委譲
	HRESULT hr = Loader::MeshLoader::LoadFromScene(scene, root, name);
	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"[MeshLoader] メッシュの読み込み失敗 '{}'\n",
			Utils::Utf8ToWstring(fullPath.string())));
		return hr;
	}

	// Textureの読み込み
	hr = Loader::TextureLoader::LoadFromScene(scene, root, false);
	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"[TextureLoader] テクスチャの読み込みに失敗 '{}'\n",
			Utils::Utf8ToWstring(fullPath.string())));
		return hr;
	}

	return S_OK;
}

template <>
inline HRESULT ResourceAPI::Load<tex>(const std::string& root, const std::string& name)
{
	HRESULT hr = Loader::TextureLoader::LoadFromFile(root, name);

	if (FAILED(hr)) {
		Utils::Error(std::format(
			L"[ResourceAPI::Load<Tex>] Failed to load texture '{}{}' (HRESULT = 0x{:08X})",
			Utils::Utf8ToWstring(root),
			Utils::Utf8ToWstring(name),
			static_cast<unsigned>(hr)
		));
		return hr;
	}

	return S_OK;
}

template<>
inline bool ResourceAPI::Has<mdl>(const std::string& name)
{
	name;
	return false;
}

template<>
inline bool ResourceAPI::Has<tex>(const std::string& name)
{
	bool exists = Resource::TextureManager::GetInstance()->Has(name);

	if (!exists) {
		Utils::Error(std::format(
			L"[ResourceAPI::Has<Tex>] Texture '{}' not found or not loaded.",
			Utils::Utf8ToWstring(name)
		));
		return false;
	}

	return true;
}

inline HRESULT ResourceAPI::SceneReset()
{
	return S_OK;
}

}
