#pragma once

#include <string>
#include <Windows.h>
#include <vector>
#include "Resource/Mesh/MeshManager.h"

// 前方宣言
namespace Assimp { class Importer; }
struct aiScene;
struct aiMesh;
struct aiMaterial;

namespace Tsumi::Loader {

/* Meshの読み込み処理 */
class MeshLoader {

public:
	/// <summary>
	/// メッシュの読み込み処理
	/// </summary>
	static HRESULT Load(const std::string& fullPath, const std::string& alias);

private:
	/// <summary>
	/// aiScene から Vertex / Index を生成
	/// </summary>
	static HRESULT LoadFromScene(
		const aiScene* scene,
		std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);

	/// <summary>
	/// aiMesh を Vertex / Index に変換
	/// </summary>
	static bool ParseAiMesh(
		const aiMesh* mesh,
		std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
};

}