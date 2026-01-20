#include "MeshManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include "Utils/Logger/Logger.h"
#include "Utils/Func/UtilFunc.h"
#include "Resource/Tex/TextureManager.h"
#include <vector>
#include <unordered_map>
#include <format>
#include <cassert>
#include <filesystem>

using namespace Tsumi::Resource;
using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;
using namespace DirectX;
namespace fs = std::filesystem;

MeshManager::MeshManager()
{
	// DX12マネージャのインスタンス取得
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

HRESULT MeshManager::RegisterMesh(
	const std::string& key, 
	const std::vector<Vertex>& vertices, 
	const std::vector<uint32_t>& indices,
	const std::string& textureKey)
{
	// すでに同じ実キーで登録済みの場合は何もしない
	// → 二重ロード防止（同一メッシュの多重GPU生成を防ぐ）
	if (HasKey(key))
		return S_OK;

	// 頂点・インデックスデータの空チェック
	if (vertices.empty() || indices.empty())
		return E_INVALIDARG;

	// ListとContextの取得と空チェック
	CommandContext* ctx = dx12Mgr_->GetUploadCmdContext();
	if (!ctx)
		return E_POINTER;
	ID3D12GraphicsCommandList* list = ctx->GetList();
	if (!list)
		return E_FAIL;

	// 登録するメッシュ実体を構築
	MeshAsset asset{};
	asset.key = key;
	asset.defaultTextureKey = textureKey; // 保存
	asset.vertexCount = static_cast<uint32_t>(vertices.size());
	asset.indexCount = static_cast<uint32_t>(indices.size());
	asset.stride = sizeof(Vertex);

	// 頂点バッファ・インデックスバッファの作成
	ComPtr<ID3D12Resource> vbUpload;
	ComPtr<ID3D12Resource> ibUpload;

	HRESULT hr = CreateVertexBuffer(list, vertices, asset, vbUpload);
	if (FAILED(hr)) return hr;
	hr = CreateIndexBuffer(list, indices, asset, ibUpload);
	if (FAILED(hr)) return hr;

	// ここで一回だけ実行＆待機
	hr = ctx->ExecuteAndWait();
	if (FAILED(hr)) return hr;
	{
		std::lock_guard lock(mutex_);
		meshes_.emplace(
			key,
			std::make_unique<MeshAsset>(std::move(asset))
		);
	}

	return S_OK;
}

void MeshManager::RegisterAlias(const std::string& alias, const std::string& key)
{
	// alias → key の対応表は共有データのためロック
	std::lock_guard lock(mutex_);

	auto it = aliasToKey_.find(alias);
	if (it != aliasToKey_.end()) {
		// すでに登録済みの alias が存在する場合、
		// 異なる key を指そうとするのは設計ミスとして扱う
		if (it->second != key) {
			assert(false && "Mesh alias collision");
		}
		// 同じ key であれば問題ないので何もしない
		return;
	}

	// 新しい alias → key の対応を登録
	aliasToKey_.emplace(alias, key);
}


void MeshManager::UnloadAll()
{
	// GPU がメッシュを使用中でないことを保証
	if (dx12Mgr_ && dx12Mgr_->GetUploadCmdContext())
		dx12Mgr_->GetUploadCmdContext()->WaitForGpu();

	std::lock_guard lock(mutex_);
	meshes_.clear();
	aliasToKey_.clear();
}

HRESULT MeshManager::CreateVertexBuffer(ID3D12GraphicsCommandList* list, const std::vector<Vertex>& vertices, 
										MeshAsset& out, Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadVB)
{
	if (!list) return E_FAIL;

	const size_t size = vertices.size() * sizeof(Vertex);

	HRESULT hr = CreateBufferFromData(
		list,
		vertices.data(),
		size,
		out.vertexBuffer,
		outUploadVB);
	if (FAILED(hr)) return hr;

	out.vbView.BufferLocation = out.vertexBuffer->GetGPUVirtualAddress();
	out.vbView.SizeInBytes = static_cast<UINT>(size);
	out.vbView.StrideInBytes = (UINT)sizeof(Vertex);

	return S_OK;
}

HRESULT MeshManager::CreateIndexBuffer(ID3D12GraphicsCommandList* list, const std::vector<uint32_t>& indices, 
									   MeshAsset& out, Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadIB)
{
	if (!list) return E_FAIL;

	const size_t size = indices.size() * sizeof(uint32_t);

	HRESULT hr = CreateBufferFromData(
		list,
		indices.data(),
		size,
		out.indexBuffer,
		outUploadIB);
	if (FAILED(hr)) return hr;

	out.ibView.BufferLocation = out.indexBuffer->GetGPUVirtualAddress();
	out.ibView.SizeInBytes = static_cast<UINT>(size);
	out.ibView.Format = DXGI_FORMAT_R32_UINT;

	return S_OK;
}

HRESULT MeshManager::CreateBufferFromData(ID3D12GraphicsCommandList* list, const void* data, size_t dataSize, 
										  Microsoft::WRL::ComPtr<ID3D12Resource>& outDefault, Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload)
{
	if (!dx12Mgr_) return E_POINTER;

	ID3D12Device* device = dx12Mgr_->GetDevice();
	if (!device || !list) return E_POINTER;

	// Default heap（GPU側）
	CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

	HRESULT hr = device->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&outDefault));
	if (FAILED(hr)) return hr;

	// Upload heap（CPU→GPU転送用）
	CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

	hr = device->CreateCommittedResource(
		&uploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&outUpload));
	if (FAILED(hr)) return hr;

	// CPU -> Upload -> Default のコピーコマンドを積む
	D3D12_SUBRESOURCE_DATA sub{};
	sub.pData = data;
	sub.RowPitch = dataSize;
	sub.SlicePitch = dataSize;

	UpdateSubresources(list, outDefault.Get(), outUpload.Get(), 0, 0, 1, &sub);

	return S_OK;
}
