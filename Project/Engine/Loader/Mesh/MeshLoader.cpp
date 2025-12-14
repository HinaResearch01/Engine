#include "MeshLoader.h"
#include "Resource/Mesh/MeshManager.h"
#include "Utils/Logger/UtilsLog.h"
#include "Utils/Func/UtilFunc.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <format>

using namespace Tsumi::Loader;
using namespace Tsumi::Resource;
namespace fs = std::filesystem;

HRESULT MeshLoader::LoadFromFile(const std::string& root, const std::string& name)
{
	if (name.empty()) return E_INVALIDARG;

	// ファイルパスと管理用キーを構築
	fs::path p(name);
	fs::path fullPath;
	if (p.is_absolute()) fullPath = p;
	else if (!root.empty()) fullPath = fs::path(root) / p;
	else fullPath = p;

	std::string key = MakeKeyFromRoot(root, name);

	// すでにロード済みであれば何もしない
	if (MeshManager::GetInstance()->Has(key)) return S_OK;

	// ファイル存在チェック
	if (!fs::exists(fullPath)) {
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

	// Assimp でモデル読み込み
	const aiScene* scene = importer.ReadFile(fullPath.string().c_str(), flags);
	if (!scene) {
		Utils::Log(std::format(
			L"[MeshLoader] Assimp 読み込み失敗: {}\n",
			Utils::Utf8ToWstring(importer.GetErrorString())));
		return E_FAIL;
	}

	// シーンの解析とメッシュ生成は LoadFromScene に委譲
	HRESULT hr = LoadFromScene(scene, root, key);
	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"[MeshLoader] メッシュ生成に失敗 '{}'\n",
			Utils::Utf8ToWstring(fullPath.string())));
		return hr;
	}

	return S_OK;
}

HRESULT MeshLoader::LoadFromScene(
	const aiScene* scene,
	const std::string& root,
	const std::string& nameKeyBase)
{
	root;
	if (!scene || !scene->HasMeshes()) return S_OK;

	HRESULT overall = S_OK;
	overall;

	// 従来挙動と同様、すべての aiMesh を 1 つのメッシュに統合する
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve(10240);
	indices.reserve(10240);
	uint32_t vertexOffset = 0;

	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
		const aiMesh* aimesh = scene->mMeshes[mi];
		std::vector<Vertex> localV;
		std::vector<uint32_t> localI;

		// Assimp メッシュを自前形式に変換
		if (!ParseAiMesh(aimesh, localV, localI))
			continue;

		// 頂点追加
		vertices.insert(vertices.end(), localV.begin(), localV.end());

		// インデックスは頂点オフセットを加算
		for (uint32_t idx : localI)
			indices.push_back(vertexOffset + idx);

		vertexOffset = static_cast<uint32_t>(vertices.size());
	}

	if (vertices.empty() || indices.empty()) {
		Utils::Log(std::format(
			L"[MeshLoader] 有効なジオメトリが存在しません '{}'\n",
			Utils::Utf8ToWstring(nameKeyBase)));
		return E_FAIL;
	}

	// GPU へのアップロードと管理は MeshManager に委譲
	return MeshManager::GetInstance()->CreateFromCpuData(
		nameKeyBase, vertices, indices);
}

bool MeshLoader::ParseAiMesh(
	const aiMesh* src,
	std::vector<Vertex>& outVertices,
	std::vector<uint32_t>& outIndices)
{
	if (!src || !src->HasPositions())
		return false;

	outVertices.resize(src->mNumVertices);
	outIndices.clear();

	// --- 頂点変換（右手系 → 左手系） ---
	for (unsigned int i = 0; i < src->mNumVertices; ++i) {
		const aiVector3D& p = src->mVertices[i];
		Vertex v{};

		// 位置（X 反転で RH → LH）
		v.pos.x = -p.x;
		v.pos.y = p.y;
		v.pos.z = p.z;

		// 法線
		if (src->HasNormals()) {
			const aiVector3D& n = src->mNormals[i];
			v.normal.x = -n.x;
			v.normal.y = n.y;
			v.normal.z = n.z;
		}

		// UV
		if (src->HasTextureCoords(0)) {
			const aiVector3D& uv = src->mTextureCoords[0][i];
			v.uv.x = uv.x;
			v.uv.y = uv.y;
		}

		outVertices[i] = v;
	}

	// インデックス（必ず三角形）
	for (unsigned int f = 0; f < src->mNumFaces; ++f) {
		const aiFace& face = src->mFaces[f];
		if (face.mNumIndices == 3) {
			outIndices.push_back(face.mIndices[0]);
			outIndices.push_back(face.mIndices[1]);
			outIndices.push_back(face.mIndices[2]);
		}
	}

	return !(outVertices.empty() || outIndices.empty());
}

std::string MeshLoader::MakeKeyFromRoot(
	const std::string& root,
	const std::string& name)
{
	fs::path p(name);
	fs::path full;

	// 絶対パスの場合
	if (p.is_absolute()) {
		if (!root.empty()) {
			try {
				fs::path rootp(root);
				fs::path rel = fs::relative(p, rootp);
				if (!rel.empty()) {
					return (rootp / rel).lexically_normal().string();
				}
			}
			catch (...) {
				// 相対化失敗時は絶対パスをそのまま使用
			}
		}
		return p.lexically_normal().string();
	}
	// 相対パスの場合
	else {
		if (!root.empty()) full = fs::path(root) / p;
		else full = p;
		return full.lexically_normal().string();
	}
}
