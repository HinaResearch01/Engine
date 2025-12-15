#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <cassert>
#include <map>
#include <vector>
#include <mutex>
#include <stb_image.h>
#include <DirectXTex.h>
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
struct TextureAsset {
	std::string key;               // 実キー（正規化パス）
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;    // GPUリソース
	Tsumi::DX12::DescAlloc srvDesc;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uint32_t width = 0;
	uint32_t height = 0;
	UINT mipLevels = 1;
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
	/// 登録
	/// </summary>
	HRESULT  RegisterTexture(const std::string& key, const ScratchImage& image, DXGI_FORMAT viewFormat);
	void RegisterAlias(const std::string& alias, const std::string& key);

	/// <summary>
	/// 既にロード済みか
	/// </summary>
	bool HasKey(const std::string& key) const {
		std::lock_guard lock(mutex_);
		return textures_.find(key) != textures_.end();
	}
	bool HasAlias(const std::string& alias) const {
		std::lock_guard lock(mutex_);
		return aliasToKey_.find(alias) != aliasToKey_.end();
	}

	/// <summary>
	/// 全アンロード
	/// </summary>
	void UnloadAll();

#pragma region Accessor
	TextureAsset* GetTexture(const std::string& key) {
		std::lock_guard lock(mutex_);
		auto it = textures_.find(key);
		return (it != textures_.end()) ? it->second.get() : nullptr;
	}
#pragma endregion 

private:
	/// <summary>
	/// GPUリソースとSRVを作成する
	/// </summary>
	HRESULT CreateFromScratchImage(const std::string& name, const DirectX::ScratchImage& mipChain, DXGI_FORMAT viewFormat);

private:
	// 実体：key（正規化パス） → Texture
	std::unordered_map<std::string, std::unique_ptr<TextureAsset>> textures_;
	// 論理名：alias → key
	std::unordered_map<std::string, std::string> aliasToKey_;

	mutable std::mutex mutex_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}