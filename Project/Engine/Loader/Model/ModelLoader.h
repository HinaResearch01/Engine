#pragma once

#include <string>
#include <Windows.h>

namespace Assimp { class Importer; }
struct aiScene;

namespace Tsumi::Loader {

/* Modelの読み込み処理 */
class ModelLoader {

public:
	/// <summary>
	/// モデルファイルを読み込み、
	/// Mesh / Texture をまとめて登録する
	/// </summary>
	static HRESULT Load(
		const std::string& fullPath,
		const std::string& alias,
		bool srgb = true);

private:
	/// <summary>
	/// 読み込み済み aiScene から各 Loader に処理を委譲
	/// </summary>
	static HRESULT LoadFromScene(
		const aiScene* scene,
		const std::string& fullPath,
		const std::string& alias,
		bool srgb);
};

}