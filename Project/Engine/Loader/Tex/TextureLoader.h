#pragma once

#include <string>
#include <Windows.h>
#include <DirectXTex.h>

// 前方宣言
struct aiScene;
struct aiMaterial;

namespace Tsumi::Loader {

/* Textureの読み込み処理 */
class TextureLoader {

public:
	/// <summary>
	/// テクスチャの読み込み処理
	/// </summary>
	static HRESULT Load(const std::string& fullPath, const std::string& alias, bool srgb = false);
	static HRESULT LoadFromScene(
		const aiScene* scene,
		const std::string& modelPath,
		const std::string& alias,
		bool srgb);

private:
	/// <summary>
	/// 走査してMgrに登録
	/// </summary>
	static HRESULT RegisterFromImage(
		const std::string& key,
		const DirectX::ScratchImage& image,
		const std::string& alias,
		bool srgb);

	/// <summary>
	/// 画像データの作成
	/// </summary>
	static HRESULT DecodeToScratchImage(const std::string& path, bool srgb, DirectX::ScratchImage& outImage);

	/// <summary>
	/// 既読チェック専用関数
	/// </summary>
	static bool TryResolveAlias(
		const std::string& key,
		const std::string& alias);
};

}