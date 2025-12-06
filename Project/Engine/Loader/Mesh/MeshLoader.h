#pragma once

#include <string>
#include <Windows.h>
#include <vector>

// 前方宣言
namespace Assimp { class Importer; }
struct aiScene;
struct aiMesh;
struct aiMaterial;
namespace Tsumi::Resource {
struct Vertex;
}

namespace Tsumi::Loader {

/* Meshの読み込み処理 */
class MeshLoader {

public:
	/// <summary>
	/// メッシュの読み込み処理
	/// </summary>
	static HRESULT LoadFromFile(const std::string& root, const std::string& name);
	
	/// <summary>
	/// aiSceneを走査してメッシュを読み込む
	/// </summary>
	static HRESULT LoadFromScene(const aiScene* scene, const std::string& root, const std::string& nameKeyBase);

private:
	/// <summary>
	/// 単一のaiMeshをパースして、頂点・インデックスを生成する
	/// </summary>
	static bool ParseAiMesh(const aiMesh* src, std::vector<Tsumi::Resource::Vertex>& outVertices, std::vector<uint32_t>& outIndices);
	
	/// <summary>
	/// 正規化キーを生成する
	/// </summary>
	static std::string MakeKeyFromRoot(const std::string& root, const std::string& name);
};

}