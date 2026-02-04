#include "TextureManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include "DX12/Desc/PersistentDescAllocator.h"
#include "Utils/Logger/Logger.h"
#include "Utils/Func/UtilFunc.h"
#include <filesystem>
#include <DirectXTex.h>

using namespace Tsumi::Resource;
using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;
using namespace DirectX;
namespace fs = std::filesystem;

static DXGI_FORMAT MakeTypelessIfNeeded(DXGI_FORMAT fmt)
{
    switch (fmt) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        default:
            return fmt;
    }
}

static DXGI_FORMAT ChooseViewFormat(DXGI_FORMAT baseFormat, bool srgb)
{
	switch (baseFormat)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
				: DXGI_FORMAT_R8G8B8A8_UNORM;

		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
				: DXGI_FORMAT_B8G8R8A8_UNORM;

		default:
			return baseFormat; // その他のフォーマットはそのまま
	}
}

TextureManager::TextureManager()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

HRESULT TextureManager::RegisterTexture(const std::string& key, const ScratchImage& image, DXGI_FORMAT viewFormat)
{
	// すでに同じ実キー（正規化パス）で登録済みの場合は何もしない
	// → 二重ロード防止（同一テクスチャの多重GPU生成を防ぐ）
	if (HasKey(key))
		return S_OK;

	if (!dx12Mgr_) return E_POINTER;

	TextureAsset asset{};
	asset.key = key;

	// Default heap texture resource を作る（COMMON）
	HRESULT hr = CreateTextureResource(image, viewFormat, asset);
	if (FAILED(hr)) return hr;

	// subresources を作る
	std::vector<D3D12_SUBRESOURCE_DATA> subres;
	hr = BuildSubresources(image, subres);
	if (FAILED(hr)) return hr;

	// upload buffer を作る
	const UINT64 uploadBytes = GetRequiredIntermediateSize(asset.resource.Get(), 0, (UINT)subres.size());

	ComPtr<ID3D12Resource> upload;
	hr = CreateUploadBuffer(uploadBytes, upload);
	if (FAILED(hr)) return hr;

	// Upload(COPY) を記録して submit
	{
		CommandContext* uploadCtx = dx12Mgr_->GetUploadCmdContext();
		if (!uploadCtx) return E_POINTER;

		hr = RecordUpload(uploadCtx,
						   asset.resource.Get(),
						   upload.Get(),
						   subres,
						   asset.currentState);
		if (FAILED(hr)) return hr;
	}

	// SRVを作る
	hr = CreateTextureSRV(image.GetMetadata(), asset);
	if (FAILED(hr)) return hr;

	// SRV用 state に遷移（DIRECT）
	hr = TransitionTextureToSRV(asset);
	if (FAILED(hr)) return hr;

	// マネージャに登録
	{
		std::lock_guard lock(mutex_);
		pendingUploads_.push_back(upload);

		textures_.emplace(
			key,
			std::make_unique<TextureAsset>(std::move(asset)));
	}

	return S_OK;
}

void TextureManager::RegisterAlias(const std::string& alias, const std::string& key)
{
	// alias → key の対応表は共有データのためロック
	std::lock_guard lock(mutex_);

	auto it = aliasToKey_.find(alias);
	if (it != aliasToKey_.end()) {
		// すでに登録済みの alias が存在する場合、
		// 異なる key を指そうとするのは設計ミスとして扱う
		if (it->second != key) {
			assert(false && "Texture alias collision");
		}
		// 同じ key であれば問題ないので何もしない
		return;
	}

	// 新しい alias → key の対応を登録
	aliasToKey_.emplace(alias, key);
}

void TextureManager::UnloadAll()
{
	if (dx12Mgr_) {
		dx12Mgr_->WaitForGpu(); // GPU完了待ち
	}

	std::lock_guard lock(mutex_);
	textures_.clear();
	aliasToKey_.clear();

	// 完了したのでアップロードバッファも解放
	pendingUploads_.clear();
}

HRESULT TextureManager::CreateTextureResource(const DirectX::ScratchImage& mipChain, DXGI_FORMAT viewFormat, TextureAsset& outAsset)
{
	if (mipChain.GetImageCount() == 0)
		return E_INVALIDARG;

	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	if (!device) return E_FAIL;

	const TexMetadata& meta = mipChain.GetMetadata();
	const UINT mipCount = meta.mipLevels ? (UINT)meta.mipLevels : 1;

	const bool isSRGB =
		viewFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
		viewFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	// SRGB の場合、resource は typeless にして SRV だけ srgb/unorm を切り替える
	const DXGI_FORMAT resourceFormat = isSRGB ? MakeTypelessIfNeeded(meta.format) : meta.format;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = meta.width;
	desc.Height = (UINT)meta.height;
	desc.DepthOrArraySize = (UINT16)meta.arraySize;
	desc.MipLevels = (UINT16)mipCount;
	desc.Format = resourceFormat;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);

	ComPtr<ID3D12Resource> texture;

	// initial state は COMMON 固定
	HRESULT hr = device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&texture));
	if (FAILED(hr)) return hr;

	outAsset.resource = texture;
	outAsset.width = (uint32_t)meta.width;
	outAsset.height = (uint32_t)meta.height;
	outAsset.mipLevels = mipCount;
	outAsset.format = viewFormat;
	outAsset.currentState = D3D12_RESOURCE_STATE_COMMON;

	return S_OK;
}

HRESULT TextureManager::CreateUploadBuffer(UINT64 uploadBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload)
{
	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	if (!device) return E_POINTER;

	CD3DX12_HEAP_PROPERTIES upHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);

	return device->CreateCommittedResource(
		&upHeap,
		D3D12_HEAP_FLAG_NONE,
		&upDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&outUpload));
}

HRESULT TextureManager::BuildSubresources(const DirectX::ScratchImage& mipChain, std::vector<D3D12_SUBRESOURCE_DATA>& outSubres)
{
	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	if (!device) return E_POINTER;

	const auto& meta = mipChain.GetMetadata();

	outSubres.clear();
	outSubres.reserve(mipChain.GetImageCount());

	// DirectXTex helper
	PrepareUpload(device, mipChain.GetImages(), mipChain.GetImageCount(), meta, outSubres);

	if (outSubres.empty())
		return E_FAIL;

	return S_OK;
}

HRESULT TextureManager::RecordUpload(Tsumi::DX12::CommandContext* uploadCtx, ID3D12Resource* dstTexture, ID3D12Resource* uploadBuffer, const std::vector<D3D12_SUBRESOURCE_DATA>& subres, D3D12_RESOURCE_STATES& inOutState)
{
	if (!uploadCtx || !dstTexture || !uploadBuffer) return E_POINTER;

	// UploadCtx を必ず open 化
	HRESULT hr = uploadCtx->BeginOneShot();
	if (FAILED(hr)) return hr;

	ID3D12GraphicsCommandList* list = uploadCtx->GetList();
	if (!list) return E_FAIL;

	UpdateSubresources(
		list,
		dstTexture,
		uploadBuffer,
		0, 0,
		(UINT)subres.size(),
		subres.data());

	// Submit & Wait
	hr = uploadCtx->EndOneShotAndWait();
	if (FAILED(hr)) return hr;

	// 暗黙 decay により、完了後の dst は COMMON
	inOutState = D3D12_RESOURCE_STATE_COMMON;
	return S_OK;
}

HRESULT TextureManager::CreateTextureSRV(const DirectX::TexMetadata& meta, TextureAsset& asset)
{
	ID3D12Device* device = dx12Mgr_ ? dx12Mgr_->GetDevice() : nullptr;
	auto* allocator = dx12Mgr_ ? dx12Mgr_->GetPersistentDescAllocator() : nullptr;
	if (!device || !allocator) return E_FAIL;
	if (!asset.resource) return E_FAIL;

	asset.srv = allocator->Allocate(1);
	if (!asset.srv.valid()) return E_FAIL;

	const bool isArray = meta.arraySize > 1;
	const bool isCube = (meta.miscFlags & TEX_MISC_TEXTURECUBE) != 0;

	const bool srgb =
		asset.format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
		asset.format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Format = ChooseViewFormat(meta.format, srgb);

	const UINT mipCount = asset.mipLevels ? asset.mipLevels : 1;

	if (isArray) {
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srv.Texture2DArray.MipLevels = mipCount;
		srv.Texture2DArray.ArraySize = (UINT)meta.arraySize;
		srv.Texture2DArray.MostDetailedMip = 0;
		srv.Texture2DArray.FirstArraySlice = 0;
		srv.Texture2DArray.ResourceMinLODClamp = 0.0f;
	}
	else if (isCube) {
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srv.TextureCube.MipLevels = mipCount;
		srv.TextureCube.MostDetailedMip = 0;
		srv.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else {
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = mipCount;
		srv.Texture2D.MostDetailedMip = 0;
		srv.Texture2D.ResourceMinLODClamp = 0.0f;
	}

	device->CreateShaderResourceView(asset.resource.Get(), &srv, asset.srv.cpu);
	return S_OK;
}

HRESULT TextureManager::TransitionTextureToSRV(TextureAsset& asset)
{
	if (!dx12Mgr_ || !asset.resource) return E_POINTER;

	auto* ctx = dx12Mgr_->GetResourceCmdContext();
	if (!ctx) return E_POINTER;

	const D3D12_RESOURCE_STATES before = asset.currentState;
	const D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// 既に SRV 状態なら何もしない
	if (before == after) return S_OK;

	HRESULT hr = TransitionResource(ctx, asset.resource.Get(), before, after);
	if (FAILED(hr)) return hr;

	// state更新は TextureManager 内だけで行う
	asset.currentState = after;
	return S_OK;
}

HRESULT TextureManager::TransitionResource(Tsumi::DX12::CommandContext* ctx, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	if (!ctx || !res) return E_POINTER;

	HRESULT hr = ctx->BeginOneShot();
	if (FAILED(hr)) return hr;

	auto* list = ctx->GetList();
	if (!list) return E_FAIL;

	// ※ Transition は DIRECT キュー（resourceCtx）で行うこと。
	D3D12_RESOURCE_BARRIER b = CD3DX12_RESOURCE_BARRIER::Transition(res, before, after);
	list->ResourceBarrier(1, &b);

	return ctx->EndOneShotAndWait();
}
