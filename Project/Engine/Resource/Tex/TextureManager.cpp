#include "TextureManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/UtilsLog.h"
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

static DXGI_FORMAT ChooseViewFormat(DXGI_FORMAT resourceFmt, bool srgb)
{
	switch (resourceFmt)
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
			return resourceFmt; // その他のフォーマットはそのまま
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

	// 登録するテクスチャ実体を構築
	TextureAsset asset{};
	asset.key = key;

	// ScratchImage（CPU側データ）から GPU リソースを作成・アップロード
	HRESULT hr = CreateTextureResource(image, viewFormat, asset);
	if (FAILED(hr))
		return hr;

	// GPU リソースに対応する SRV（ディスクリプタ）を作成
	hr = CreateTextureSRV(image.GetMetadata(), asset);
	if (FAILED(hr))
		return hr;

	// 作成がすべて成功した時点でマネージャに登録
	// ※ GPU リソース生成中に失敗した場合、ここには到達しない
	{
		std::lock_guard lock(mutex_);
		textures_.emplace(
			key,
			std::make_unique<TextureAsset>(std::move(asset))
		);
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
	// 永続ディスクリプタ用アロケータを取得
	auto* allocator = dx12Mgr_->GetPersistentDescAlloc();

	// 現在のフレームインデックス（遅延解放用）
	uint32_t frameIndex = 0;
	if (dx12Mgr_->GetFrameSync())
		frameIndex = dx12Mgr_->GetFrameSync()->GetFrameIndex();

	{
		// テクスチャ実体・alias 管理テーブルをまとめて操作するためロック
		std::lock_guard lock(mutex_);

		if (allocator) {
			// 各テクスチャが保持している SRV ディスクリプタを遅延解放キューへ
			// → GPU がまだ参照中でも安全に解放できる
			for (auto& [name, tex] : textures_) {
				if (tex && tex->srvDesc.valid()) {
					allocator->DeferFree(tex->srvDesc, frameIndex);
				}
			}
		}

		// テクスチャ実体をすべて破棄
		textures_.clear();

		// alias → key の対応もすべて破棄
		// → Scene 切り替え時などにゴースト参照が残らないようにする
		aliasToKey_.clear();
	}
}

HRESULT TextureManager::CreateTextureResource(const DirectX::ScratchImage& mipChain, DXGI_FORMAT viewFormat, TextureAsset& outAsset)
{
	using namespace DirectX;

	if (mipChain.GetImageCount() == 0)
		return E_INVALIDARG;

	ID3D12Device* device = dx12Mgr_->GetDevice();
	CommandContext* ctx = dx12Mgr_->GetCommandContext();
	if (!device || !ctx)
		return E_FAIL;

	const TexMetadata& meta = mipChain.GetMetadata();
	UINT mipCount = meta.mipLevels ? static_cast<UINT>(meta.mipLevels) : 1;

	// sRGB 対応（Resource と View を分ける）
	bool isSRGB =
		viewFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
		viewFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	DXGI_FORMAT resourceFormat =
		isSRGB ? MakeTypelessIfNeeded(meta.format) : meta.format;

	// Resource desc
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = meta.width;
	desc.Height = static_cast<UINT>(meta.height);
	desc.DepthOrArraySize = static_cast<UINT16>(meta.arraySize);
	desc.MipLevels = static_cast<UINT16>(mipCount);
	desc.Format = resourceFormat;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	ComPtr<ID3D12Resource> texture;
	CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture));

	if (FAILED(hr)) return hr;

	// Upload
	std::vector<D3D12_SUBRESOURCE_DATA> subres;
	PrepareUpload(device,
				  mipChain.GetImages(),
				  mipChain.GetImageCount(),
				  meta,
				  subres);

	UINT64 uploadBytes =
		GetRequiredIntermediateSize(texture.Get(), 0, (UINT)subres.size());

	ComPtr<ID3D12Resource> upload;
	CD3DX12_HEAP_PROPERTIES upHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);

	hr = device->CreateCommittedResource(
		&upHeap,
		D3D12_HEAP_FLAG_NONE,
		&upDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&upload));

	if (FAILED(hr)) return hr;

	UpdateSubresources(
		ctx->GetList(),
		texture.Get(),
		upload.Get(),
		0, 0,
		(UINT)subres.size(),
		subres.data());

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		texture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	ctx->GetList()->ResourceBarrier(1, &barrier);
	hr = ctx->ExecuteAndWait();
	if (FAILED(hr)) return hr;

	// out
	outAsset.resource = texture;
	outAsset.width = (uint32_t)meta.width;
	outAsset.height = (uint32_t)meta.height;
	outAsset.mipLevels = mipCount;
	outAsset.format = viewFormat;

	return S_OK;
}

HRESULT TextureManager::CreateTextureSRV(const DirectX::TexMetadata& meta, TextureAsset& asset)
{
	ID3D12Device* device = dx12Mgr_->GetDevice();
	DescriptorAllocator* allocator = dx12Mgr_->GetPersistentDescAlloc();
	if (!device || !allocator)
		return E_FAIL;

	asset.srvDesc = allocator->Allocate(1);
	if (!asset.srvDesc.valid())
		return E_FAIL;

	bool isArray = meta.arraySize > 1;
	bool isCube = (meta.miscFlags & TEX_MISC_TEXTURECUBE) != 0;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Format = ChooseViewFormat(meta.format, asset.format);

	if (isArray)
	{
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srv.Texture2DArray.MipLevels = asset.mipLevels;
		srv.Texture2DArray.ArraySize = (UINT)meta.arraySize;
	}
	else if (isCube)
	{
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srv.TextureCube.MipLevels = asset.mipLevels;
	}
	else
	{
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = asset.mipLevels;
	}

	device->CreateShaderResourceView(
		asset.resource.Get(),
		&srv,
		asset.srvDesc.cpuHandle);

	return S_OK;
}