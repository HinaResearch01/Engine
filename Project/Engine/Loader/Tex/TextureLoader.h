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
	
	/// <summary>
	/// 
	/// </summary>
	static HRESULT ExtractSceneTextures(
		const aiScene* scene, const std::string& fullPath, const std::string& alias, bool srgb = false);

private:
	/// <summary>
	/// 画像データの作成
	/// </summary>
	static HRESULT DecodeToScratchImage(const std::string& path, bool srgb, DirectX::ScratchImage& outImage);
};

}