#include "ModelLoader.h"

#include "Loader/Mesh/MeshLoader.h"
#include "Loader/Tex/TextureLoader.h"
#include "Utils/Func/UtilFunc.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <filesystem>

using namespace Tsumi::Loader;
namespace fs = std::filesystem;

HRESULT ModelLoader::Load(const std::string& fullPath, const std::string& alias, bool srgb)
{
	if (fullPath.empty())
		return E_INVALIDARG;

	if (!fs::exists(fullPath))
		return E_FAIL;

	Assimp::Importer importer;

	uint32_t flags =
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace;

	const aiScene* scene = importer.ReadFile(fullPath, flags);
	if (!scene)
		return E_FAIL;

	return LoadFromScene(scene, fullPath, alias, srgb);
}

HRESULT ModelLoader::LoadFromScene(const aiScene* scene, const std::string& fullPath, const std::string& alias, bool srgb)
{
	if (!scene)
		return E_FAIL;

	const std::string modelKey = Utils::Func::MakeKeyFromRoot("", fullPath);

	HRESULT hr = S_OK;

	// 1) Texture
	if (scene->HasMaterials())
	{
		hr = TextureLoader::LoadFromScene(scene, fullPath, alias, srgb);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// 2) Mesh
	if (scene->HasMeshes())
	{
		hr = MeshLoader::LoadFromScene(scene, modelKey, alias);
		if (FAILED(hr))
			return hr;
	}

	return S_OK;
}
