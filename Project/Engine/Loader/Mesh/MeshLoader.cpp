#include "MeshLoader.h"
#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <format>

using namespace Tsumi::Loader;
using namespace Tsumi::Resource;

HRESULT MeshLoader::Load(const std::string& fullPath, const std::string& alias)
{
	// パスの空チェック
	if (fullPath.empty())
		return E_INVALIDARG;

	// key = 正規化パス（TextureLoader と同一ルール）
	std::string key = Utils::MakeKeyFromRoot("", fullPath);
	
	// 既読チェック
	if (TryResolveAlias(key, alias))
		return S_OK;

	// ファイル存在チェック
	if (!std::filesystem::exists(fullPath))
		return E_FAIL;

	Assimp::Importer importer;
	// 読み込み時の後処理フラグ
	// ・三角形化
	// ・右手系 → 左手系用に並び順を反転
	// ・UV の上下反転
	// ・接線空間計算
	const unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_FlipWindingOrder |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace;

	// メッシュファイル読み込み
	const aiScene* scene =
		importer.ReadFile(fullPath, flags);

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

	// GPU リソースの生成・登録は MeshManager に任せる
	hr = meshMgr->RegisterMesh(key, vertices, indices);
	if (FAILED(hr))
		return hr;

	// alias → key の関連付けを登録
	meshMgr->RegisterAlias(alias, key);
	return S_OK;
}

HRESULT MeshLoader::ParseScene(const aiScene* scene, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	// 不正チェック
	if (!scene || !scene->HasMeshes())
		return E_FAIL;

	uint32_t vertexOffset = 0;

	// シーン内の全メッシュを順番に処理
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		const aiMesh* mesh = scene->mMeshes[meshIndex];

		std::vector<Vertex> localVertices;
		std::vector<uint32_t> localIndices;

		// aiMesh を CPU メッシュデータに変換
		if (!ParseAiMesh(mesh, localVertices, localIndices))
			continue;

		// 頂点データをまとめて追加
		outVertices.insert(
			outVertices.end(),
			localVertices.begin(),
			localVertices.end());

		// インデックスは頂点オフセットを加算して追加
		for (uint32_t idx : localIndices)
			outIndices.push_back(vertexOffset + idx);

		vertexOffset += static_cast<uint32_t>(localVertices.size());
	}

	// 有効なメッシュが1つもなかった場合は失敗
	if (outVertices.empty() || outIndices.empty())
		return E_FAIL;

	return S_OK;
}

bool MeshLoader::ParseAiMesh(const aiMesh* mesh, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	// 頂点が存在しないメッシュは無効
	if (!mesh || !mesh->HasPositions())
		return false;

	// 頂点数分の領域を確保
	outVertices.resize(mesh->mNumVertices);

	// 頂点データの変換
	for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
		Vertex v{};

		// position（右手系 → 左手系）
		const aiVector3D& pos = mesh->mVertices[i];
		v.pos = { -pos.x, pos.y, pos.z };

		// normal（存在する場合のみ）
		if (mesh->HasNormals()) {
			const aiVector3D& n = mesh->mNormals[i];
			v.normal = { -n.x, n.y, n.z };
		}

		// UV（0番目のUVセットのみ使用）
		if (mesh->HasTextureCoords(0)) {
			const aiVector3D& uv = mesh->mTextureCoords[0][i];
			v.uv = { uv.x, uv.y };
		}

		outVertices[i] = v;
	}

	// インデックスデータ（三角形のみ想定）
	outIndices.clear();
	for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
		const aiFace& face = mesh->mFaces[f];
		if (face.mNumIndices == 3) {
			outIndices.push_back(face.mIndices[0]);
			outIndices.push_back(face.mIndices[1]);
			outIndices.push_back(face.mIndices[2]);
		}
	}

	return true;
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
