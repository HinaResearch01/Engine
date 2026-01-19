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
	// ・右手系 → 左手系用に並び順を反転
	// ・UV の上下反転
	// ・接線空間計算
	const unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |   // 右手→左手、winding/transform含めて整合
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace;

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
	// 不正チェック
		// ・scene が存在すること
		// ・root node があること（Assimp のシーングラフ前提）
		// ・少なくとも 1 つ以上の mesh を含むこと
	if (!scene || !scene->mRootNode || !scene->HasMeshes())
		return E_FAIL;

	// 出力バッファを初期化
	outVertices.clear();
	outIndices.clear();

	// 複数 mesh を 1 つの Vertex / Index バッファにまとめるためのオフセット
	uint32_t vertexOffset = 0;

	// ---------------------------------------------
	// ノードを再帰的に辿るためのラムダ
	//
	// parent:
	//   ルートから現在の node までに累積された transform
	// ---------------------------------------------
	std::function<void(const aiNode*, const aiMatrix4x4&)> visit =
		[&](const aiNode* node, const aiMatrix4x4& parent)
	{
		// 親の transform と、この node 自身の local transform を合成
		// → ルートから見た「この node のグローバル transform」
		const aiMatrix4x4 global = parent * node->mTransformation;

		// Assimp の行列をエンジンの Mat4x4 に変換
		//（row-major・左手系・エンジン規約）
		const Math::Mat4x4 M = ToMat4x4(global);

		// -----------------------------------------
		// この node が参照している mesh を処理
		// -----------------------------------------
		for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
			const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			if (!mesh || !mesh->HasPositions())
				continue;

			// この mesh の頂点が始まる base index
			const uint32_t baseVertex = vertexOffset;

			// -------------------------
			// Vertex の展開
			// -------------------------
			for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
				Vertex dst{};

				// position
				// ・Assimp で ConvertToLeftHanded 済み
				// ・node transform をここで bake する
				const aiVector3D& p = mesh->mVertices[v];
				dst.pos = TransformPoint(M, { p.x, p.y, p.z });

				// normal
				// ・非一様スケール対応のため逆転置で変換
				if (mesh->HasNormals()) {
					const aiVector3D& n = mesh->mNormals[v];
					dst.normal = TransformNormal(M, { n.x, n.y, n.z });
				}

				// UV（0 番目のみ使用）
				if (mesh->HasTextureCoords(0)) {
					const aiVector3D& uv = mesh->mTextureCoords[0][v];
					dst.uv = { uv.x, uv.y };
				}

				outVertices.push_back(dst);
			}

			// -------------------------
			// Index の展開
			// ・三角形化は Assimp 側で保証
			// ・vertexOffset を加算して結合
			// -------------------------
			for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
				const aiFace& face = mesh->mFaces[f];
				if (face.mNumIndices != 3)
					continue;

				outIndices.push_back(baseVertex + face.mIndices[0]);
				outIndices.push_back(baseVertex + face.mIndices[1]);
				outIndices.push_back(baseVertex + face.mIndices[2]);
			}

			// 次の mesh 用にオフセットを進める
			vertexOffset += mesh->mNumVertices;
		}

		// -----------------------------------------
		// 子ノードを再帰的に処理
		// -----------------------------------------
		for (unsigned int c = 0; c < node->mNumChildren; ++c)
			visit(node->mChildren[c], global);
	};

	// ルートから traversal 開始
	// aiMatrix4x4 のデフォルトコンストラクタは identity
	aiMatrix4x4 identity;
	visit(scene->mRootNode, identity);

	// 有効なジオメトリが生成できていなければ失敗
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
		a.a1, a.a2, a.a3, a.a4,
		a.b1, a.b2, a.b3, a.b4,
		a.c1, a.c2, a.c3, a.c4,
		a.d1, a.d2, a.d3, a.d4
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
