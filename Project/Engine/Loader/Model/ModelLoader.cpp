#include "ModelLoader.h"
#include "Loader/Mesh/MeshLoader.h"
#include "Loader/Tex/TextureLoader.h"
#include "Utils/Logger/UtilsLog.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <filesystem>

using namespace Tsumi::Loader;
namespace fs = std::filesystem;
using namespace DirectX;

HRESULT ModelLoader::Load(const std::string& fullPath, const std::string& alias, bool srgb)
{
	// パスチェック
	if (fullPath.empty())
		return E_INVALIDARG;

	if (!fs::exists(fullPath))
		return E_FAIL;

	Assimp::Importer importer;

	// Assimp 後処理フラグ
	const unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_FlipWindingOrder |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace;

	// モデル読み込み（ここは 1 回だけ）
	const aiScene* scene = importer.ReadFile(fullPath, flags);
	if (!scene)
		return E_FAIL;

	// aiScene を各 Loader に流す
	return LoadFromScene(scene, fullPath, alias, srgb);
}

HRESULT ModelLoader::LoadFromScene(const aiScene* scene, const std::string& fullPath, const std::string& alias, bool srgb)
{
	if (!scene)
		return E_FAIL;

	HRESULT hr = S_OK;

	// -------------------------
	// Mesh 登録
	// -------------------------
	if (scene->HasMeshes()) {
		hr = MeshLoader::LoadFromScene(
			scene,
			fullPath,
			alias);
		if (FAILED(hr))
			return hr;
	}

	// -------------------------
	// Texture 登録
	// -------------------------
	if (scene->HasMaterials()) {
		hr = TextureLoader::LoadFromScene(
			scene,
			fullPath,
			alias,
			srgb);
		if (FAILED(hr))
			return hr;
	}

	return S_OK;
}