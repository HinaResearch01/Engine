#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <cassert>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <stb_image.h>
#include <DirectXTex.h>
#include "DX12/Desc/DescriptorUtils.h"

namespace TexFileExt {
const std::pair<std::string, uint32_t> PNG = { ".png", 0 };
const std::pair<std::string, uint32_t> JPEG = { ".jpeg", 1 };
const std::pair<std::string, uint32_t> DDS = { ".dds", 2 };
}

// 前方宣言
namespace Tsumi::DX12 {
class DX12Manager;
class CommandContext;
}

namespace tme::sys::resource {

struct TextureAsset {
	std::string key; // 実キー（正規化パス）
	// gpuリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	Tsumi::DX12::DescriptorHandle srv{};
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uint32_t width = 0;
	uint32_t height = 0;
	UINT mipLevels = 1;

	// State Tracking
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
};

/* テクスチャ管理 */
class TextureManager {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	TextureManager();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~TextureManager() = default;

	/// <summary>
	/// 登録
	/// </summary>
	HRESULT  RegisterTexture(
		const std::string& key,
		const DirectX::ScratchImage& image, 
		DXGI_FORMAT viewFormat);
	void RegisterAlias(const std::string& alias, const std::string& key);

	/// <summary>
	/// 既読か
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

	/// <summary>
	/// 保持しているアップロードバッファを解放する
	/// </summary>
	void ReleaseUploadBuffers() {
		std::lock_guard lock(mutex_);
		pendingUploads_.clear();
	}

#pragma region Accessor
	TextureAsset* GetTexture(const std::string& key) {
		std::lock_guard lock(mutex_);
		
		// 1. 実キーで検索
		if (auto it = textures_.find(key); it != textures_.end()) {
			return it->second.get();
		}

		// 2. Alias解決
		if (auto it = aliasToKey_.find(key); it != aliasToKey_.end()) {
			if (auto tit = textures_.find(it->second); tit != textures_.end()) {
				return tit->second.get();
			}
		}

		return nullptr;
	}

	std::vector<std::string> GetTextureNames() const {
		std::lock_guard lock(mutex_);
		std::vector<std::string> names;
		names.reserve(textures_.size());
		for (const auto& [key, _] : textures_) {
			names.push_back(key);
		}
		return names;
	}
#pragma endregion 

private:
	/// <summary>
	/// GPU リソース作成（Default heap）
	///  - initial state は COMMON 固定
	/// </summary>
	HRESULT CreateTextureResource(
		const DirectX::ScratchImage& mipChain,
		DXGI_FORMAT viewFormat,
		TextureAsset& outAsset);

	/// <summary>
	/// Upload用の中間バッファを作る
	/// </summary>
	HRESULT CreateUploadBuffer(
		UINT64 uploadBytes,
		Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload);

	/// <summary>
	/// サブリソース情報を生成
	/// </summary>
	HRESULT BuildSubresources(
		const DirectX::ScratchImage& mipChain,
		std::vector<D3D12_SUBRESOURCE_DATA>& outSubres);

	/// <summary>
	/// Upload(COPY) でデータ転送を積む
	///  - COMMON -> COPY_DEST を COPY側で確実に行ってから UpdateSubresources
	/// </summary>
	HRESULT RecordUpload(
		Tsumi::DX12::CommandContext* uploadCtx,
		ID3D12Resource* dstTexture,
		ID3D12Resource* uploadBuffer,
		const std::vector<D3D12_SUBRESOURCE_DATA>& subres,
		D3D12_RESOURCE_STATES& inOutState);

	/// <summary>
	/// SRVの作成（Persistent allocator）
	/// </summary>
	HRESULT CreateTextureSRV(
		const DirectX::TexMetadata& meta,
		TextureAsset& asset);

	/// <summary>
	/// SRV 用 state へ遷移する（DIRECT）
	/// </summary>
	HRESULT TransitionTextureToSRV(TextureAsset& asset);

	/// <summary>
	/// state遷移
	/// </summary>
	HRESULT TransitionResource(
		Tsumi::DX12::CommandContext* ctx,
		ID3D12Resource* res,
		D3D12_RESOURCE_STATES before,
		D3D12_RESOURCE_STATES after);


private:
	// 実体：key（正規化パス） → Texture
	std::unordered_map<std::string, std::unique_ptr<TextureAsset>> textures_;
	// 論理名：alias → key
	std::unordered_map<std::string, std::string> aliasToKey_;

	// 実行待ちのアップロードバッファを保持するリスト
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingUploads_;

	mutable std::mutex mutex_;
	Tsumi::DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}