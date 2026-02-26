#pragma once

#include <string>
#include <Windows.h>
#include <vector>
#include "Resource/ResourceSystem.h"
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
	static HRESULT LoadFromScene(
		const aiScene* scene,
		const std::string& key,
		const std::string& alias);

private:
	/// <summary>
	/// 走査してMgrに登録
	/// </summary>
	static HRESULT RegisterFromScene(
		const aiScene* scene,
		const std::string& key,
		const std::string& alias);

	/// <summary>
	/// aiScene から Vertex / Index を生成
	/// </summary>
	static HRESULT ParseScene(
		const aiScene* scene,
		std::vector<tme::sys::resource::Vertex>& outVertices, std::vector<uint32_t>& outIndices);

	/// <summary>
	/// 既読チェック専用関数
	/// </summary>
	static bool TryResolveAlias(
		const std::string& key,
		const std::string& alias);

	/// <summary>
	/// transform を適用してメッシュをパース
	/// </summary>
	static Math::Mat4x4 ToMat4x4(const aiMatrix4x4& a);

	/// <summary>
	/// 
	/// </summary>
	static Math::Vec3f TransformPoint(const Math::Mat4x4& m, const Math::Vec3f& p);

	/// <summary>
	/// 
	/// </summary>
	static Math::Vec3f TransformNormal(const Math::Mat4x4& m, const Math::Vec3f& n);
};

}