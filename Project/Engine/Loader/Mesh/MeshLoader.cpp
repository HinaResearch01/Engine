#include "MeshLoader.h"
#include "Utils/Logger/Logger.h"
#include "Utils/Func/UtilFunc.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <format>
#include <functional>

using namespace Tsumi::Loader;
using namespace Tsumi::Resource;

HRESULT MeshLoader::Load(const std::string& fullPath, const std::string& alias)
{
	// パスの空チェック
	if (fullPath.empty())
		return E_INVALIDARG;

	// key = 正規化パス（TextureLoader と同一ルール）
	std::string key = Utils::Func::MakeKeyFromRoot("", fullPath);
	
	// 既読チェック
	if (TryResolveAlias(key, alias))
		return S_OK;

	// ファイル存在チェック
	if (!std::filesystem::exists(fullPath))
		return E_FAIL;

	Assimp::Importer importer;
	// 読み込み時の後処理フラグ
	// ・三角形化
	// ・左手系変換 (MakeLeftHanded | FlipUVs | FlipWindingOrder)
	// ・接線空間計算
	// ・ノード変換を頂点にベイク (PreTransformVertices)
	const unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace |
		aiProcess_PreTransformVertices;

	// aiProcess_FlipWindingOrder は不要
	const aiScene* scene = importer.ReadFile(fullPath, flags);

	// 読み込み失敗、またはメッシュが含まれていない場合は失敗
	if (!scene || !scene->HasMeshes())
		return E_FAIL;

	// 登録処理
	return RegisterFromScene(scene, key, alias);
}

HRESULT MeshLoader::LoadFromScene(const aiScene* scene, const std::string& key, const std::string& alias)
{
	// 読み込み失敗、またはメッシュが含まれていない場合は失敗
	if (!scene || !scene->HasMeshes())
		return E_FAIL;

	// 既読チェック
	if (TryResolveAlias(key, alias))
		return S_OK;

	// 登録処理
	return RegisterFromScene(scene, key, alias);
}

HRESULT MeshLoader::RegisterFromScene(const aiScene* scene, const std::string& key, const std::string& alias)
{
	auto* meshMgr = ResourceSystem::GetInstance()->GetMeshManager();
	
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	// aiScene から Vertex / Index を生成
	HRESULT hr = ParseScene(scene, vertices, indices);
	if (FAILED(hr))
		return hr;

	// デフォルトテクスチャのキーを予測
	std::string defaultTexKey = "";
	if (scene->mNumMeshes > 0) {
		unsigned int matIdx = scene->mMeshes[0]->mMaterialIndex;
		defaultTexKey = alias + "_diffuse_" + std::to_string(matIdx);
	}

	// GPU リソースの生成・登録は MeshManager に任せる
	hr = meshMgr->RegisterMesh(key, vertices, indices, defaultTexKey);
	if (FAILED(hr))
		return hr;

	// alias → key の関連付けを登録
	meshMgr->RegisterAlias(alias, key);
	return S_OK;
}

HRESULT MeshLoader::ParseScene(const aiScene* scene, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	if (!scene || !scene->HasMeshes())
		return E_FAIL;

	outVertices.clear();
	outIndices.clear();

	uint32_t vertexOffset = 0;

	// PreTransformVertices を使用しているため、ノード階層を辿る必要はない
	// mMeshes に全てのメッシュが（変換済みで）格納されている
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
		const aiMesh* mesh = scene->mMeshes[i];
		if (!mesh || !mesh->HasPositions())
			continue;

		// -------------------------
		// Vertex の展開
		// -------------------------
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
			Vertex dst{};

			// position
			// Assimpで既に Transform & LeftHanded済み
			const aiVector3D& p = mesh->mVertices[v];
			dst.pos = { p.x, p.y, p.z };

			// normal
			if (mesh->HasNormals()) {
				const aiVector3D& n = mesh->mNormals[v];
				dst.normal = { n.x, n.y, n.z };
			}

			// UV
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D& uv = mesh->mTextureCoords[0][v];
				dst.uv = { uv.x, uv.y };
			}

			outVertices.push_back(dst);
		}

		// -------------------------
		// Index の展開
		// -------------------------
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
			const aiFace& face = mesh->mFaces[f];
			if (face.mNumIndices != 3)
				continue;

			outIndices.push_back(vertexOffset + face.mIndices[0]);
			outIndices.push_back(vertexOffset + face.mIndices[1]);
			outIndices.push_back(vertexOffset + face.mIndices[2]);
		}

		vertexOffset += mesh->mNumVertices;
	}

	return (outVertices.empty() || outIndices.empty()) ? E_FAIL : S_OK;
}

bool MeshLoader::TryResolveAlias(const std::string& key, const std::string& alias)
{
	auto* meshMgr = ResourceSystem::GetInstance()->GetMeshManager();

	if (meshMgr->HasKey(key)) {
		meshMgr->RegisterAlias(alias, key);
		return true; // すでに登録済み
	}
	return false;
}

Tsumi::Math::Mat4x4 MeshLoader::ToMat4x4(const aiMatrix4x4& a)
{
	return Math::Mat4x4(
		a.a1, a.b1, a.c1, a.d1,
		a.a2, a.b2, a.c2, a.d2,
		a.a3, a.b3, a.c3, a.d3,
		a.a4, a.b4, a.c4, a.d4
	);
}

Tsumi::Math::Vec3f MeshLoader::TransformPoint(const Math::Mat4x4& m, const Math::Vec3f& p)
{
	return m.TransformPoint(p);
}

Tsumi::Math::Vec3f MeshLoader::TransformNormal(const Math::Mat4x4& m, const Math::Vec3f& n)
{
	Math::Mat4x4 invT = m.Inverse().Transpose();
	return invT.TransformVector(n).Normalized();
}
