#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <cassert>
#include <map>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
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

struct Mesh {
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};
	UINT vertexCount = 0;
	UINT indexCount = 0;
	UINT vertexStride = sizeof(Vertex);
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;
};

/* メッシュ管理 */
class MeshManager {

private: // シングルトン
	MeshManager();
	~MeshManager() = default;
	MeshManager(const MeshManager&) = delete;
	const MeshManager& operator=(const MeshManager&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static MeshManager* GetInstance() {
		static MeshManager instance;
		return &instance;
	}

	/// <summary>
	/// 読み込み処理
	/// </summary>
	HRESULT Load(const std::string& root, const std::string& name);

	/// <summary>
	/// 既にロード済みか
	/// </summary>
	bool Has(const std::string& name) {
		return meshes_.find(name) != meshes_.end();
	}

	/// <summary>
	/// 全アンロード（シーンリセット等）
	/// </summary>
	void UnloadAll();

#pragma region Accessor
	Mesh* GetMesh(const std::string& name) {
		auto it = meshes_.find(name);
		return (it != meshes_.end()) ? it->second.get() : nullptr;
	}
#pragma endregion

private:
	// helper to create GPU buffers (unchanged behavior)
	HRESULT CreateBufferFromData(const void* data, size_t dataSize, ID3D12Resource** outDefault, ID3D12Resource** outUpload);

	// parse a single aiMesh into outVertices/outIndices (per-mesh)
	bool ParseAiMesh(const aiMesh* aimesh, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);

	// extracts diffuse texture path (if any) from material
	std::string GetDiffuseTexturePath(const aiMaterial* material);

private:
	std::map<std::string, std::unique_ptr<Mesh>> meshes_;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}