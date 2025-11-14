#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <cassert>
#include <map>
#include <vector>
#include <stb_image.h>
#include "DX12/Desc/DescriptorAllocator.h"

namespace Tsumi::DX12 {
class DX12Manager;
}

namespace TexFileExt {
const std::pair<std::string, uint32_t> PNG = { ".png", 0 };
const std::pair<std::string, uint32_t> JPEG = { ".jpeg", 1 };
const std::pair<std::string, uint32_t> DSS = { ".dds", 2 };
}

namespace Tsumi::Resource {

struct Texture {
	Microsoft::WRL::ComPtr<ID3D12Resource>	resource;
	Tsumi::DX12::DescAlloc srvDesc;	// SRV descriptor (CPU/GPU handles)
	UINT width = 0;
	UINT height = 0;
	UINT mipLevels = 1;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

/* テクスチャ管理 */
class TextureManager {

private: // シングルトン
	TextureManager();
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
	bool Load(const std::string& root, const std::string& name, bool srgb = false);

	/// <summary>
	/// 既にロード済みか
	/// </summary>
	bool Has(const std::string& name) const {
		return textures_.find(name) != textures_.end();
	}

	/// <summary>
	/// 全アンロード（シーンリセット等）
	/// </summary>
	void UnloadAll();


#pragma region Accessor
	Texture* GetTexture(const std::string& name) {
		auto it = textures_.find(name);
		return (it != textures_.end()) ? it->second.get() : nullptr;
	}
#pragma endregion 

private:
	/// <summary>
	/// メモリからテクスチャを生成
	/// </summary>
	Texture* CreateTextureFromMemory(
		const std::string& name,
		const uint8_t* pixels,
		UINT width, UINT height,
		DXGI_FORMAT format,
		UINT bytesPerPixel
	);

private:
	std::map<std::string, std::unique_ptr<Texture>> textures_;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}