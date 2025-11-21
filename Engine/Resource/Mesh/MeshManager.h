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
	/// Mapに追加
	/// </summary>
	HRESULT Emplace(const std::string& name, std::unique_ptr<Mesh> mesh, bool overwrite = true);

	/// <summary>
	/// 既にロード済みか
	/// </summary>
	bool Has(const std::string& name) const {
		std::lock_guard lock(mutex_);
		return meshes_.find(name) != meshes_.end();
	}

	/// <summary>
	/// 全アンロード
	/// </summary>
	void UnloadAll();

	/// <summary>
	/// CPU 側の頂点/インデックス配列から GPU バッファを作成して管理する API
	/// </summary>
	HRESULT CreateFromCpuData(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

#pragma region Accessor
	Mesh* GetMesh(const std::string& name) {
		std::lock_guard lock(mutex_);
		auto it = meshes_.find(name);
		return (it != meshes_.end()) ? it->second.get() : nullptr;
	}
#pragma endregion

private:
	// helper to create GPU buffers
	HRESULT CreateBufferFromData(const void* data, size_t dataSize, ID3D12Resource** outDefault, ID3D12Resource** outUpload);

private:
	std::map<std::string, std::unique_ptr<Mesh>> meshes_;
	mutable std::mutex mutex_;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}