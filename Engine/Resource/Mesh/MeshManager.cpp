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
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Tsumi::Resource;
using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;
using namespace DirectX;
namespace fs = std::filesystem;

MeshManager::MeshManager()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

HRESULT MeshManager::Load(const std::string& root, const std::string& name)
{
	if (Has(name)) return S_OK;   // すでに読み込み済み

	// --- パスを結合 ---
	fs::path filepath = fs::path(root) / name;

	// --- ファイル存在チェック ---
	if (!fs::exists(filepath)) {
		Utils::Log(std::format(L"[MeshManager] ファイルが存在しません: {}\n",
			Utils::Utf8ToWstring(filepath.string())));
		return E_FAIL;
	}

	//------------------------------------------------------------
	// Assimp でモデルを読み込む
	//------------------------------------------------------------
	Assimp::Importer importer;
	const unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace;

	const aiScene* scene = importer.ReadFile(filepath.string().c_str(), flags);
	if (!scene) {
		Utils::Log(std::format(L"[MeshManager] Assimp 読み込み失敗: {}\n",
			Utils::Utf8ToWstring(importer.GetErrorString())));
		return E_FAIL;
	}
	if (!scene->HasMeshes()) {
		Utils::Log(std::format(L"[MeshManager] メッシュが存在しません: {}\n",
			Utils::Utf8ToWstring(filepath.string())));
		return E_FAIL;
	}

	//------------------------------------------------------------
	// 全メッシュを 1 つの Mesh に統合
	//------------------------------------------------------------
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve(10240);
	indices.reserve(10240);

	uint32_t vertexOffset = 0;

	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
	{
		const aiMesh* aimesh = scene->mMeshes[mi];
		std::vector<Vertex> localV;
		std::vector<uint32_t> localI;

		if (!ParseAiMesh(aimesh, localV, localI))
			continue;

		// --- 頂点を追加 ---
		vertices.insert(vertices.end(), localV.begin(), localV.end());

		// --- インデックスのオフセット ---
		for (uint32_t idx : localI)
			indices.push_back(vertexOffset + idx);

		vertexOffset = static_cast<uint32_t>(vertices.size());

		//------------------------------------------------------------
		// テクスチャ読み込み（最初のメッシュのみ）
		//------------------------------------------------------------
		if (mi == 0 && scene->HasMaterials()) {
			const aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];
			std::string texPath = GetDiffuseTexturePath(material);

			if (!texPath.empty()) {
				HRESULT hr = TextureManager::GetInstance()->Load(root, texPath, false);
				if (FAILED(hr)) {
					Utils::Log(std::format(L"[MeshManager] テクスチャ読み込み失敗: {}\n",
						Utils::Utf8ToWstring(texPath)));
				}
			}
		}
	}

	if (vertices.empty() || indices.empty()) {
		Utils::Log(std::format(L"[MeshManager] 有効なジオメトリがありません: {}\n",
			Utils::Utf8ToWstring(filepath.string())));
		return E_FAIL;
	}

	//------------------------------------------------------------
	// GPU バッファ生成
	//------------------------------------------------------------
	ComPtr<ID3D12Resource> vbDefault, vbUpload;
	ComPtr<ID3D12Resource> ibDefault, ibUpload;

	size_t vbSize = vertices.size() * sizeof(Vertex);
	size_t ibSize = indices.size() * sizeof(uint32_t);

	HRESULT hr = CreateBufferFromData(vertices.data(), vbSize, &vbDefault, &vbUpload);
	if (FAILED(hr)) return hr;

	hr = CreateBufferFromData(indices.data(), ibSize, &ibDefault, &ibUpload);
	if (FAILED(hr)) return hr;

	//------------------------------------------------------------
	// Mesh オブジェクト作成
	//------------------------------------------------------------
	auto mesh = std::make_unique<Mesh>();
	mesh->vertexCount = static_cast<UINT>(vertices.size());
	mesh->indexCount = static_cast<UINT>(indices.size());
	mesh->vertexStride = sizeof(Vertex);

	mesh->vertexBuffer = vbDefault;
	mesh->indexBuffer = ibDefault;

	mesh->vbView.BufferLocation = vbDefault->GetGPUVirtualAddress();
	mesh->vbView.SizeInBytes = (UINT)vbSize;
	mesh->vbView.StrideInBytes = mesh->vertexStride;

	mesh->ibView.BufferLocation = ibDefault->GetGPUVirtualAddress();
	mesh->ibView.Format = DXGI_FORMAT_R32_UINT;
	mesh->ibView.SizeInBytes = (UINT)ibSize;

	meshes_.emplace(name, std::move(mesh));

	return S_OK;
}

void MeshManager::UnloadAll()
{
	if (dx12Mgr_ && dx12Mgr_->GetCommandContext())
		dx12Mgr_->GetCommandContext()->WaitForGpu();

	meshes_.clear();
}

HRESULT MeshManager::CreateBufferFromData(const void* data, size_t size, ID3D12Resource** outDefault, ID3D12Resource** outUpload)
{
	if (!dx12Mgr_) return E_POINTER;

	ID3D12Device* device = dx12Mgr_->GetDevice();
	CommandContext* ctx = dx12Mgr_->GetCommandContext();
	if (!device || !ctx) return E_POINTER;

	//------------------------------------------------------------
	// Default Heap（GPUメモリ）
	//------------------------------------------------------------
	ComPtr<ID3D12Resource> defaultBuf;
	CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

	HRESULT hr = device->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr, IID_PPV_ARGS(&defaultBuf));

	if (FAILED(hr)) {
		Utils::Log(std::format(L"[MeshManager] DefaultBuffer 作成失敗 (hr=0x{:08X})", (unsigned)hr));
		return hr;
	}

	//------------------------------------------------------------
	// Upload Heap（CPU → GPU 転送用）
	//------------------------------------------------------------
	ComPtr<ID3D12Resource> uploadBuf;
	CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

	hr = device->CreateCommittedResource(
		&uploadHeap, D3D12_HEAP_FLAG_NONE,
		&uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&uploadBuf));

	if (FAILED(hr)) {
		Utils::Log(std::format(L"[MeshManager] UploadBuffer 作成失敗 (hr=0x{:08X})", (unsigned)hr));
		return hr;
	}

	//------------------------------------------------------------
	// UpdateSubresources で GPU に転送
	//------------------------------------------------------------
	D3D12_SUBRESOURCE_DATA sub{};
	sub.pData = data;
	sub.RowPitch = size;
	sub.SlicePitch = size;

	ID3D12GraphicsCommandList* list = ctx->GetList();
	if (!list) return E_FAIL;

	UpdateSubresources(list, defaultBuf.Get(), uploadBuf.Get(), 0, 0, 1, &sub);

	// コピー完了 → GPUリソース状態変更 → 待機
	hr = ctx->ExecuteAndWait();
	if (FAILED(hr)) {
		Utils::Log(std::format(L"[MeshManager] ExecuteAndWait 失敗 (hr=0x{:08X})", (unsigned)hr));
		return hr;
	}

	*outDefault = defaultBuf.Detach();
	*outUpload = uploadBuf.Detach();
	return S_OK;
}

bool MeshManager::ParseAiMesh(const aiMesh* src, std::vector<Vertex>& outV,	std::vector<uint32_t>& outI)
{
	if (!src || !src->HasPositions())
		return false;

	outV.resize(src->mNumVertices);
	outI.clear();

	// --- 頂点変換（右手系 → 左手系） ---
	for (unsigned int i = 0; i < src->mNumVertices; ++i)
	{
		const aiVector3D& p = src->mVertices[i];
		Vertex v{};

		v.pos.x = -p.x;   // X反転で RH→LH
		v.pos.y = p.y;
		v.pos.z = p.z;

		// 法線
		if (src->HasNormals()) {
			const aiVector3D& n = src->mNormals[i];
			v.normal.x = -n.x;
			v.normal.y = n.y;
			v.normal.z = n.z;
		}

		// UV
		if (src->HasTextureCoords(0)) {
			const aiVector3D& uv = src->mTextureCoords[0][i];
			v.uv.x = uv.x;
			v.uv.y = uv.y;
		}

		outV[i] = v;
	}

	// --- インデックス（三角形前提） ---
	for (unsigned int f = 0; f < src->mNumFaces; ++f)
	{
		const aiFace& face = src->mFaces[f];
		if (face.mNumIndices == 3) {
			outI.push_back(face.mIndices[0]);
			outI.push_back(face.mIndices[1]);
			outI.push_back(face.mIndices[2]);
		}
	}

	return !(outV.empty() || outI.empty());
}

std::string MeshManager::GetDiffuseTexturePath(const aiMaterial* material)
{
	if (!material) return {};

	aiString path;
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
			return std::string(path.C_Str());
		}
	}
	return {};
}
