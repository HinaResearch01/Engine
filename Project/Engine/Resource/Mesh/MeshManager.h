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
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <mutex>
#include "Math/TMath.h"

namespace Tsumi::DX12 {
class DX12Manager;
}

namespace Tsumi::Resource {

struct Vertex {
	Math::Vec3f pos;
	Math::Vec3f normal;
	Math::Vec2f uv;
};

struct MeshAsset {
	std::string key;   // 実キー（正規化パス）
	// gpuリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	// view情報
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};
	UINT vertexCount = 0;
	UINT indexCount = 0;
	UINT stride = sizeof(Vertex);
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;
};

/* メッシュ管理 */
class MeshManager {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	MeshManager();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~MeshManager() = default;

	/// <summary>
	/// 登録
	/// </summary>
	HRESULT RegisterMesh(
		const std::string& key,
		const std::vector<Vertex>& vertices,
		const std::vector<uint32_t>& indices);
	void RegisterAlias(const std::string& alias, const std::string& key);

	/// <summary>
	/// 既にロード済みか
	/// </summary>
	bool HasKey(const std::string& key) const {
		std::lock_guard lock(mutex_);
		return meshes_.find(key) != meshes_.end();
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
	MeshAsset* GetMesh(const std::string& name) {
		std::lock_guard lock(mutex_);
		auto it = meshes_.find(name);
		return (it != meshes_.end()) ? it->second.get() : nullptr;
	}
#pragma endregion

private:
	/// <summary>
	/// 
	/// </summary>
	HRESULT CreateVertexBuffer(
		ID3D12GraphicsCommandList* list, const std::vector<Vertex>& vertices,
		MeshAsset& out, Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadVB);

	/// <summary>
	/// 
	/// </summary>
	HRESULT CreateIndexBuffer(
		ID3D12GraphicsCommandList* list, const std::vector<uint32_t>& indices,
		MeshAsset& out, Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadIB);

	/// <summary>
	/// 
	/// </summary>
	HRESULT CreateBufferFromData(
		ID3D12GraphicsCommandList* list,
		const void* data, size_t dataSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& outDefault,
		Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload);
	
private:
	// 実体：key（正規化パス） → Mesh
	std::unordered_map<std::string, std::unique_ptr<MeshAsset>> meshes_;
	// 論理名：alias → key
	std::unordered_map<std::string, std::string> aliasToKey_;

	mutable std::mutex mutex_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}