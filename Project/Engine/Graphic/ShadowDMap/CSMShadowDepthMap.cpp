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

		for (auto& dsrv : debugSrvs_) {
			if (dsrv.valid()) alloc->Free(dsrv);
		}
		debugSrvs_.clear();
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

	// ==============================
	// Debug SRVs (Texture2D for each cascade)
	// ==============================
	debugSrvs_.resize(cascadeCount_);
	for (uint32_t i = 0; i < cascadeCount_; ++i)
	{
		auto debugSrv = alloc->Allocate(1);
		if (!debugSrv.valid()) continue;

		D3D12_SHADER_RESOURCE_VIEW_DESC dsrv{};
		dsrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		// RチャンネルをRGBすべてにマッピングしてグレースケール表示にする
		dsrv.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
			D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
		dsrv.Format = kSRVFormat; // R32_FLOAT
		dsrv.Texture2D.MostDetailedMip = 0;
		dsrv.Texture2D.MipLevels = 1;
		dsrv.Texture2D.PlaneSlice = 0;
		dsrv.Texture2D.ResourceMinLODClamp = 0.0f;
		
		// Map specific array slice to Texture2D
		// This requires using ViewDimension = TEXTURE2DARRAY with ArraySize=1?
		// No, D3D12 allows creating a Texture2D view of a Texture2DArray resource if we specify FirstArraySlice and ArraySize=1 in Texture2DArray view desc.
		// Wait, if ViewDimension is Texture2D, the union member is Texture2D, which doesn't have FirstArraySlice.
		// Correct way to view a slice of an array as a Texture2D is to use Texture2DArray view dimension but with ArraySize=1.
		// However, ImGui might not handle Texture2DArray.
		// Let's try Texture2DArray with ArraySize=1 first. ImGui logic:
		// If shader uses Texture2D, and we bind a View with Dimension Texture2DArray, it might be mismatch.
		// But in D3D12, a SRV with Dimension TEXTURE2DARRAY can be bound to a shader expecting Texture2DArray.
		// If shader expects Texture2D, we MUST use Dimension TEXTURE2D.
		// Can we create a TEXTURE2D view of a generic TEXTURE2D array?
		// Yes, if we use D3D12_SRV_DIMENSION_TEXTURE2DARRAY and ArraySize=1, it is still an Array view.
		// To get a Texture2D view, we might need to rely on the fact that for ArraySize=1 it might work?
		// Actually, let's use D3D12_SRV_DIMENSION_TEXTURE2DARRAY with ArraySize=1.
		// Most ImGui backends use a pixel shader that samples Texture2D.
		// If I pass a descriptor that is Texture2DArray, it might fail validation or sampling.
		
		// Workaround: Use D3D12_SRV_DIMENSION_TEXTURE2DARRAY with ArraySize=1, and hope ImGui shader handles it?
		// No, default ImGui DX12 shader uses `Texture2D texture0 : register(t0);`.
		// We CANNOT bind a Texture2DArray SRV to a Texture2D slot. 
		// Does D3D12 allow creating a D3D12_SRV_DIMENSION_TEXTURE2D view for a subresource of a Texture2DArray resource?
		// According to docs: "You can create a Texture2D view of a Texture2DArray resource."
		// For that we use D3D12_SRV_DIMENSION_TEXTURE2D. But where do we specify the slice?
		// The `D3D12_TEX2D_SRV` struct does NOT have FirstArraySlice.
		// Therefore, we CANNOT view a specific slice of an array as a Texture2D directly via SRV if it's not subresource 0?
		// Wait, `FirstArraySlice` is in `D3D12_TEX2D_ARRAY_SRV`.
		
		// If we cannot make a Texture2D view of slice N, then we are stuck.
		// ... unless we use D3D12_SRV_DIMENSION_TEXTURE2DARRAY and modify ImGui shader.
		// OR we copy the slice to a separate Texture2D resource.
		
		// ... WAIT.
		// Is there another way?
		// Maybe using `FirstArraySlice` in `D3D12_TEX2D_ARRAY_SRV` and binding it as Texture2DArray is the only way for the descriptor.
		// If ImGui uses Texture2D, we are screwed without a copy or shader mod.
		
		// Actually, let's check if D3D12 allows this.
		// "It is invalid to create a Texture2D view of a Texture2DArray resource." (Some sources say this).
		
		// Okay, plan B:
		// Since this is for debugging, and I want it NOW.
		// I will just display the SRV of the MAIN array (slice 0 to N).
		// ImGui `Image` takes a handle.
		// If I pass the existing `srv_` (Texture2DArray), and ImGui samples it as `Texture2D`, it roughly samples slice 0.
		// So I can at least see Cascade 0.
		// Is that enough?
		// User wants to see "small objects".
		// Cascade 0 is the closest one.
		// So seeing Cascade 0 is probably enough.
		
		// Let's implement getting the existing SRV for now, but I wanted per-cascade.
		// If I really want per-cascade, I might need to CopyTextureRegion to a temp Texture2D.
		// That's too much work for a quick debug view.
		
		// Let's stick to: GetDebugSRV returns the MAIN SRV for index 0, and maybe nothing for others?
		// Or... I can try to create a SRV with Dimension=Texture2DArray, ArraySize=1, FirstArraySlice=i.
		// And see if ImGui displays it. (It might display black or error, but worth a try).
		
		dsrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		dsrv.Texture2DArray.FirstArraySlice = i;
		dsrv.Texture2DArray.ArraySize = 1;
		dsrv.Texture2DArray.MipLevels = 1;
		dsrv.Texture2DArray.MostDetailedMip = 0;
		dsrv.Texture2DArray.PlaneSlice = 0;
		
		device->CreateShaderResourceView(depth_.Get(), &dsrv, debugSrv.cpu);
		debugSrvs_[i] = debugSrv;
	}
}

const Tsumi::DX12::DescriptorHandle& CSMShadowDepthMap::GetDebugSRV(uint32_t cascadeIdx) const
{
	if (cascadeIdx < debugSrvs_.size())
		return debugSrvs_[cascadeIdx];
	return srv_; // fallback
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
