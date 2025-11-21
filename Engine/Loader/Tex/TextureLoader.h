#pragma once

#include <string>
#include <Windows.h>

// 前方宣言
struct aiScene;
struct aiMaterial;

namespace Tsumi::Loader {

/* Textureの読み込み処理 */
class TextureLoader {

public:
	/// <summary>
	/// テクスチャファイルの読み込み処理
	/// </summary>
	static HRESULT LoadFromFile(const std::string& root, const std::string& name, bool srgb = false);

	/// <summary>
	/// aiSceneを走査しテクスチャファイルを読み込む
	/// </summary>
	static HRESULT LoadFromScene(const aiScene* scene, const std::string& root, bool srgb = false);

	/// <summary>
	/// aiMaterialに含まれるテクスチャファイルを読み込む
	/// </summary>
	static HRESULT LoadFromMaterial(const aiMaterial* mat, const std::string& root, bool srgb = false);

};

}