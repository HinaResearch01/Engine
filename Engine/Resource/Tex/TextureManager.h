#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <cassert>
#include <map>
#include<vector>

namespace Tsumi::Resource {

/* テクスチャ管理 */
class TextureManager {

private: // シングルトン
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager&) = delete;
	const TextureManager& operator=(const TextureManager&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static TextureManager* GetInstance() {
		static TextureManager instance;
		return &instance;
	}

	/// <summary>
	/// 読み込み処理
	/// </summary>
	void Load(const std::string& root, const std::string& name);

private:




};

}