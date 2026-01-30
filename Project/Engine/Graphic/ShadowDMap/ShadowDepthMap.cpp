#include "ShadowDepthMap.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/PersistentDescAllocator.h"
#include "DX12/FrameSync/FrameSync.h"
#include "Utils/Logger/Logger.h"
#include <cassert>

using namespace Tsumi::Graphic;
using namespace Tsumi::DX12;

static DXGI_FORMAT MakeTypelessDepth_(DXGI_FORMAT dsvFmt)
{
	// D32 -> R32 view を作るため typeless resource にする
	if (dsvFmt == DXGI_FORMAT_D32_FLOAT) return DXGI_FORMAT_R32_TYPELESS;
	return dsvFmt; 
}

void ShadowDepthMap::Init(uint32_t size)
{
	assert(size > 0);
	size_ = 0; // Create で更新
	Create(size);
}

void ShadowDepthMap::Resize(uint32_t size)
{
	assert(size > 0);

	if (size_ == size) return;

	// Resize は「GPU が使ってない」保証が要るので安全側に倒す
	auto* dx12 = Tsumi::DX12::DX12Manager::GetInstance();
	if (dx12) dx12->WaitForGpu();

	// SRV を persistent から確保してるので、作り直す前に Free
	if (srv_.valid()) {
		auto* alloc = dx12->GetPersistentDescAllocator();
		if (alloc) alloc->Free(srv_);
		srv_ = {};
	}

	depth_.Reset();
	dsvHeap_.Reset();
	dsvCpu_ = { 0 };

	Create(size);

}

void ShadowDepthMap::Create(uint32_t size)
{
	auto* dx12 = Tsumi::DX12::DX12Manager::GetInstance();
	assert(dx12);

	ID3D12Device* device = dx12->GetDevice();
	assert(device);

	// ---- depth resource ----
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = size;
	desc.Height = size;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = MakeTypelessDepth_(kDSVFormat); // typeless
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clear{};
	clear.Format = kDSVFormat;
	clear.DepthStencil.Depth = 1.0f;
	clear.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear,
		IID_PPV_ARGS(&depth_)
	);
	assert(SUCCEEDED(hr));
	if (FAILED(hr)) {
		Utils::Logger::Error("ShadowDepthMap: CreateCommittedResource failed\n");
		return;
	}

	size_ = size;

	// ---- views ----
	CreateViews();
}

void ShadowDepthMap::CreateViews()
{
	auto* dx12 = Tsumi::DX12::DX12Manager::GetInstance();
	assert(dx12);

	ID3D12Device* device = dx12->GetDevice();
	assert(device);
	assert(depth_);

	// ==============================
	// DSV heap (1個だけ)
	// ==============================
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_));
	assert(SUCCEEDED(hr));
	if (FAILED(hr)) return;

	dsvCpu_ = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
	dsv.Format = kDSVFormat;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depth_.Get(), &dsv, dsvCpu_);

	// ==============================
	// SRV (persistent, global heap)
	// ==============================
	auto* alloc = dx12->GetPersistentDescAllocator();
	assert(alloc);

	srv_ = alloc->Allocate(1);
	assert(srv_.valid());
	if (!srv_.valid()) {
		Tsumi::Utils::Logger::Error("ShadowDepthMap: SRV Allocate failed\n");
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Format = kSRVFormat; // R32_FLOAT
	srv.Texture2D.MipLevels = 1;
	srv.Texture2D.MostDetailedMip = 0;
	srv.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(depth_.Get(), &srv, srv_.cpu);
}
