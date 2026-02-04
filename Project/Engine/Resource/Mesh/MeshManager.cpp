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
	// すでに同じ実キー（正規化パス）で登録済みの場合は何もしない
	// → 二重ロード防止（同一テクスチャの多重GPU生成を防ぐ）
	if (HasKey(key))
		return S_OK;

	if (vertices.empty() || indices.empty())
		return E_INVALIDARG;

	auto asset = std::make_unique<MeshAsset>();
	asset->key = key;
	asset->defaultTextureKey = textureKey;
	asset->vertexCount = (UINT)vertices.size();
	asset->indexCount = (UINT)indices.size();

	ComPtr<ID3D12Resource> vbUpload;
	ComPtr<ID3D12Resource> ibUpload;

	// ---- Create + Upload ----
	HRESULT hr = CreateVertexBuffer(vertices, *asset, vbUpload);
	if (FAILED(hr)) return hr;

	hr = CreateIndexBuffer(indices, *asset, ibUpload);
	if (FAILED(hr)) return hr;

	// ---- Transition to draw ----
	hr = TransitionToDrawState(*asset);
	if (FAILED(hr)) return hr;

	{
		std::lock_guard lock(mutex_);
		pendingUploads_.push_back(vbUpload);
		pendingUploads_.push_back(ibUpload);
		meshes_[key] = std::move(asset);
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
	pendingUploads_.clear();
}

HRESULT MeshManager::CreateVertexBuffer(const std::vector<Vertex>& vertices, MeshAsset& asset, Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload)
{
	const size_t size = vertices.size() * sizeof(Vertex);

	HRESULT hr = CreateDefaultBuffer(size, asset.vertexBuffer);
	if (FAILED(hr)) return hr;

	hr = CreateUploadBuffer(size, outUpload);
	if (FAILED(hr)) return hr;

	hr = UploadBuffer(
		asset.vertexBuffer.Get(),
		outUpload.Get(),
		vertices.data(),
		size,
		asset.currentVBState);
	if (FAILED(hr)) return hr;

	asset.vbView.BufferLocation = asset.vertexBuffer->GetGPUVirtualAddress();
	asset.vbView.SizeInBytes = (UINT)size;
	asset.vbView.StrideInBytes = sizeof(Vertex);

	return S_OK;
}

HRESULT MeshManager::CreateIndexBuffer(const std::vector<uint32_t>& indices, MeshAsset& asset, Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload)
{
	const size_t size = indices.size() * sizeof(uint32_t);

	HRESULT hr = CreateDefaultBuffer(size, asset.indexBuffer);
	if (FAILED(hr)) return hr;

	hr = CreateUploadBuffer(size, outUpload);
	if (FAILED(hr)) return hr;

	hr = UploadBuffer(
		asset.indexBuffer.Get(),
		outUpload.Get(),
		indices.data(),
		size,
		asset.currentIBState);
	if (FAILED(hr)) return hr;

	asset.ibView.BufferLocation = asset.indexBuffer->GetGPUVirtualAddress();
	asset.ibView.SizeInBytes = (UINT)size;
	asset.ibView.Format = DXGI_FORMAT_R32_UINT;

	return S_OK;
}

HRESULT MeshManager::CreateDefaultBuffer(size_t size, Microsoft::WRL::ComPtr<ID3D12Resource>& outDefault)
{
	ID3D12Device* device = dx12Mgr_->GetDevice();
	if (!device) return E_POINTER;

	CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

	// initial state は COMMON
	return device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&outDefault));
}

HRESULT MeshManager::CreateUploadBuffer(size_t size, Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload)
{
	ID3D12Device* device = dx12Mgr_->GetDevice();
	if (!device) return E_POINTER;

	CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

	return device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&outUpload));
}

HRESULT MeshManager::UploadBuffer(ID3D12Resource* dst, ID3D12Resource* upload, const void* data, size_t size, D3D12_RESOURCE_STATES& inOutState)
{
	auto* ctx = dx12Mgr_->GetUploadCmdContext();
	if (!ctx) return E_POINTER;

	HRESULT hr = ctx->BeginOneShot();
	if (FAILED(hr)) return hr;

	auto* list = ctx->GetList();

	// COMMON → COPY_DEST
	if (inOutState != D3D12_RESOURCE_STATE_COPY_DEST) {
		auto b = CD3DX12_RESOURCE_BARRIER::Transition(
			dst, inOutState, D3D12_RESOURCE_STATE_COPY_DEST);
		list->ResourceBarrier(1, &b);
		inOutState = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	D3D12_SUBRESOURCE_DATA sub{};
	sub.pData = data;
	sub.RowPitch = size;
	sub.SlicePitch = size;

	UpdateSubresources(list, dst, upload, 0, 0, 1, &sub);

	return ctx->EndOneShotAndWait();
}

HRESULT MeshManager::TransitionToDrawState(MeshAsset& asset)
{
	auto* ctx = dx12Mgr_->GetResourceCmdContext();
	if (!ctx) return E_POINTER;

	HRESULT hr = ctx->BeginOneShot();
	if (FAILED(hr)) return hr;

	auto* list = ctx->GetList();

	D3D12_RESOURCE_BARRIER bs[2] = {
		CD3DX12_RESOURCE_BARRIER::Transition(
			asset.vertexBuffer.Get(),
			asset.currentVBState,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
		CD3DX12_RESOURCE_BARRIER::Transition(
			asset.indexBuffer.Get(),
			asset.currentIBState,
			D3D12_RESOURCE_STATE_INDEX_BUFFER),
	};

	list->ResourceBarrier(2, bs);

	asset.currentVBState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	asset.currentIBState = D3D12_RESOURCE_STATE_INDEX_BUFFER;

	return ctx->EndOneShotAndWait();
}
