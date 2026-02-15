#include "CSMShadowDepthMap.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include "DX12/Desc/PersistentDescAllocator.h"
#include "Utils/Logger/Logger.h"
#include <d3dx12.h>
#include <cassert>

using namespace Tsumi::Graphic;
using namespace Tsumi::DX12;

DXGI_FORMAT CSMShadowDepthMap::MakeTypelessDepth_(DXGI_FORMAT dsvFmt)
{
	if (dsvFmt == DXGI_FORMAT_D32_FLOAT) return DXGI_FORMAT_R32_TYPELESS;
	return dsvFmt;
}

void CSMShadowDepthMap::Init(uint32_t size, uint32_t cascadeCount)
{
	assert(size > 0);
	assert(cascadeCount > 0);
	size_ = 0;
	cascadeCount_ = 0;
	Create(size, cascadeCount);
}

void CSMShadowDepthMap::Resize(uint32_t size, uint32_t cascadeCount)
{
	assert(size > 0);
	assert(cascadeCount > 0);

	if (size_ == size && cascadeCount_ == cascadeCount) return;

	auto* dx12 = DX12Manager::GetInstance();
	if (dx12) dx12->WaitForGpu();

	// SRVは persistent なので作り直し前にFree
	if (srv_.valid())
	{
		auto* alloc = dx12->GetPersistentDescAllocator();
		if (alloc) alloc->Free(srv_);
		srv_ = {};
	}

	depth_.Reset();
	dsvHeap_.Reset();
	dsvCpu_.clear();
	dsvIncSize_ = 0;

	Create(size, cascadeCount);
}

void CSMShadowDepthMap::Create(uint32_t size, uint32_t cascadeCount)
{
	auto* dx12 = DX12Manager::GetInstance();
	assert(dx12);

	ID3D12Device* device = dx12->GetDevice();
	assert(device);

	// ---- depth array resource ----
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = size;
	desc.Height = size;
	desc.DepthOrArraySize = static_cast<UINT16>(cascadeCount);
	desc.MipLevels = 1;
	desc.Format = MakeTypelessDepth_(kDSVFormat);               // typeless
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
	if (FAILED(hr) || !depth_)
	{
		Tsumi::Utils::Logger::Error("CSMShadowDepthMap: CreateCommittedResource failed");
		assert(false);
		return;
	}

	size_ = size;
	cascadeCount_ = cascadeCount;

	CreateViews();
}

void CSMShadowDepthMap::CreateViews()
{
	auto* dx12 = DX12Manager::GetInstance();
	assert(dx12);

	ID3D12Device* device = dx12->GetDevice();
	assert(device);
	assert(depth_);

	// ==============================
	// DSV heap（cascadeCount個）
	// ==============================
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = cascadeCount_;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_));
	if (FAILED(hr) || !dsvHeap_)
	{
		Tsumi::Utils::Logger::Error("CSMShadowDepthMap: Create DSV heap failed");
		assert(false);
		return;
	}

	dsvIncSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	dsvCpu_.resize(cascadeCount_);
	auto base = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

	for (uint32_t i = 0; i < cascadeCount_; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h = base;
		h.ptr += static_cast<SIZE_T>(i) * static_cast<SIZE_T>(dsvIncSize_);
		dsvCpu_[i] = h;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
		dsv.Format = kDSVFormat;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsv.Flags = D3D12_DSV_FLAG_NONE;
		dsv.Texture2DArray.MipSlice = 0;
		dsv.Texture2DArray.FirstArraySlice = i; 
		dsv.Texture2DArray.ArraySize = 1;       

		device->CreateDepthStencilView(depth_.Get(), &dsv, h);
	}

	// ==============================
	// SRV（Texture2DArray, persistent）
	// ==============================
	auto* alloc = dx12->GetPersistentDescAllocator();
	assert(alloc);

	srv_ = alloc->Allocate(1);
	if (!srv_.valid())
	{
		Tsumi::Utils::Logger::Error("CSMShadowDepthMap: SRV Allocate failed");
		assert(false);
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Format = kSRVFormat; // R32_FLOAT
	srv.Texture2DArray.MostDetailedMip = 0;
	srv.Texture2DArray.MipLevels = 1;
	srv.Texture2DArray.FirstArraySlice = 0;
	srv.Texture2DArray.ArraySize = cascadeCount_;
	srv.Texture2DArray.PlaneSlice = 0;
	srv.Texture2DArray.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(depth_.Get(), &srv, srv_.cpu);
}

D3D12_CPU_DESCRIPTOR_HANDLE CSMShadowDepthMap::GetDSV(uint32_t cascadeIdx) const
{
	assert(cascadeIdx < cascadeCount_);
	return dsvCpu_[cascadeIdx];
}

const D3D12_CPU_DESCRIPTOR_HANDLE* CSMShadowDepthMap::GetDSVPtr(uint32_t cascadeIdx) const
{
	assert(cascadeIdx < cascadeCount_);
	return &dsvCpu_[cascadeIdx];
}

void CSMShadowDepthMap::TransitionToWrite(DX12::CommandContext& cmd)
{
	if (!depth_) return;
	if (currentState_ == D3D12_RESOURCE_STATE_DEPTH_WRITE) return;

	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = depth_.Get();
	b.Transition.StateBefore = currentState_;
	b.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmd.GetList()->ResourceBarrier(1, &b);
	currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
}

void CSMShadowDepthMap::TransitionToRead(DX12::CommandContext& cmd)
{
	if (!depth_) return;
	if (currentState_ == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) return;

	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = depth_.Get();
	b.Transition.StateBefore = currentState_;
	b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmd.GetList()->ResourceBarrier(1, &b);
	currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}
