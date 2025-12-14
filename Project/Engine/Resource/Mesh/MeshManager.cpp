#include "MeshManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include "Utils/Logger/UtilsLog.h"
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

HRESULT MeshManager::Emplace(const std::string& name, std::unique_ptr<Mesh> mesh, bool overwrite)
{
	if (!mesh) return E_INVALIDARG;

	// meshes_ へのアクセスを排他制御
	std::lock_guard lock(mutex_);

	auto it = meshes_.find(name);
	if (it != meshes_.end()) {
		if (!overwrite) return E_FAIL;

		// 既存リソースを置き換える前に GPU の処理完了を待つ
		if (dx12Mgr_ && dx12Mgr_->GetCommandContext()) {
			dx12Mgr_->GetCommandContext()->WaitForGpu();
		}

		// 既存の GPU リソースを解放
		// （unique_ptr の破棄時に ComPtr::Reset が呼ばれる）
		it->second->vertexBuffer.Reset();
		it->second->indexBuffer.Reset();
		meshes_.erase(it);
	}

	// 新しい Mesh を登録（所有権を移動）
	meshes_.emplace(name, std::move(mesh));
	return S_OK;
}

void MeshManager::UnloadAll()
{
	// GPU がメッシュを使用中でないことを保証
	if (dx12Mgr_ && dx12Mgr_->GetCommandContext())
		dx12Mgr_->GetCommandContext()->WaitForGpu();

	std::lock_guard lock(mutex_);
	meshes_.clear();
}

HRESULT MeshManager::CreateFromCpuData(const std::string& name,
	const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indices)
{
	if (vertices.empty() || indices.empty()) {
		Utils::Log(std::format(
			L"[MeshManager] CreateFromCpuData 無効なジオメトリ '{}'\n",
			Utils::Utf8ToWstring(name)));
		return E_INVALIDARG;
	}

	ComPtr<ID3D12Resource> vbDefault, vbUpload;
	ComPtr<ID3D12Resource> ibDefault, ibUpload;

	size_t vbSize = vertices.size() * sizeof(Vertex);
	size_t ibSize = indices.size() * sizeof(uint32_t);

	// 頂点バッファを CPU データから作成
	HRESULT hr = CreateBufferFromData(vertices.data(), vbSize, &vbDefault, &vbUpload);
	if (FAILED(hr)) return hr;

	// インデックスバッファを CPU データから作成
	hr = CreateBufferFromData(indices.data(), ibSize, &ibDefault, &ibUpload);
	if (FAILED(hr)) return hr;

	// Mesh オブジェクト生成
	auto mesh = std::make_unique<Mesh>();
	mesh->vertexCount = static_cast<UINT>(vertices.size());
	mesh->indexCount = static_cast<UINT>(indices.size());
	mesh->vertexStride = sizeof(Vertex);

	mesh->vertexBuffer = vbDefault;
	mesh->indexBuffer = ibDefault;

	// 頂点バッファビュー設定
	mesh->vbView.BufferLocation = vbDefault->GetGPUVirtualAddress();
	mesh->vbView.SizeInBytes = (UINT)vbSize;
	mesh->vbView.StrideInBytes = mesh->vertexStride;

	// インデックスバッファビュー設定
	mesh->ibView.BufferLocation = ibDefault->GetGPUVirtualAddress();
	mesh->ibView.Format = DXGI_FORMAT_R32_UINT;
	mesh->ibView.SizeInBytes = (UINT)ibSize;

	// 排他制御下で登録
	{
		std::lock_guard lock(mutex_);
		meshes_.emplace(name, std::move(mesh));
	}

	return S_OK;
}

HRESULT MeshManager::CreateBufferFromData(
	const void* data,
	size_t dataSize,
	ID3D12Resource** outDefault,
	ID3D12Resource** outUpload)
{
	if (!dx12Mgr_) return E_POINTER;

	ID3D12Device* device = dx12Mgr_->GetDevice();
	CommandContext* ctx = dx12Mgr_->GetCommandContext();
	if (!device || !ctx) return E_POINTER;

	// GPU 用の Default ヒープバッファ作成
	ComPtr<ID3D12Resource> defaultBuf;
	CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

	HRESULT hr = device->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&defaultBuf));

	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"[MeshManager] DefaultBuffer 作成失敗 (hr=0x{:08X})",
			(unsigned)hr));
		return hr;
	}

	// CPU から書き込むための Upload ヒープバッファ作成
	ComPtr<ID3D12Resource> uploadBuf;
	CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

	hr = device->CreateCommittedResource(
		&uploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuf));

	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"[MeshManager] UploadBuffer 作成失敗 (hr=0x{:08X})",
			(unsigned)hr));
		return hr;
	}

	// CPU データを GPU に転送するためのサブリソース情報設定
	D3D12_SUBRESOURCE_DATA sub{};
	sub.pData = data;
	sub.RowPitch = dataSize;
	sub.SlicePitch = dataSize;

	ID3D12GraphicsCommandList* list = ctx->GetList();
	if (!list) return E_FAIL;

	// Upload → Default へコピー
	UpdateSubresources(list, defaultBuf.Get(), uploadBuf.Get(), 0, 0, 1, &sub);

	// コマンド実行＆完了待ち
	hr = ctx->ExecuteAndWait();
	if (FAILED(hr)) {
		Utils::Log(std::format(
			L"[MeshManager] ExecuteAndWait 失敗 (hr=0x{:08X})",
			(unsigned)hr));
		return hr;
	}

	// 呼び出し元へ所有権を渡す
	*outDefault = defaultBuf.Detach();
	*outUpload = uploadBuf.Detach();
	return S_OK;
}
