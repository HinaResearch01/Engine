#include "SpotShadowDepthMap.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include <cassert>

using namespace Tsumi::DX12;
using namespace Tsumi::Graphic;

void SpotShadowDepthMap::Init(uint32_t size, uint32_t count)
{
	Create(size, count);
}

void SpotShadowDepthMap::Resize(uint32_t size, uint32_t count)
{
	if (size_ == size && count_ == count) return;
	Create(size, count);
}

D3D12_CPU_DESCRIPTOR_HANDLE SpotShadowDepthMap::GetDSV(uint32_t index) const
{
	if (index >= count_) return { 0 };
	return dsvCpu_[index];
}

const D3D12_CPU_DESCRIPTOR_HANDLE* SpotShadowDepthMap::GetDSVPtr(uint32_t index) const
{
	if (index >= count_) return nullptr;
	return &dsvCpu_[index];
}

void SpotShadowDepthMap::TransitionToWrite(CommandContext& cmd)
{
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

void SpotShadowDepthMap::TransitionToRead(CommandContext& cmd)
{
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

void SpotShadowDepthMap::Create(uint32_t size, uint32_t count)
{
	size_ = size;
	count_ = count;

	auto* dx12 = DX12Manager::GetInstance();
	auto device = dx12->GetDevice();

	// 1. Resource (Texture2DArray)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = size;
		desc.Height = size;
		desc.DepthOrArraySize = static_cast<UINT16>(count);
		desc.MipLevels = 1;
		desc.Format = MakeTypelessDepth_(kDSVFormat); // R32_TYPELESS
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = kDSVFormat;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(depth_.ReleaseAndGetAddressOf())
		);

		currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depth_->SetName(L"SpotShadowDepthMapArray");
	}

	// 2. Create Views
	CreateViews();
}

void SpotShadowDepthMap::CreateViews()
{
	auto* dx12 = DX12Manager::GetInstance();
	auto device = dx12->GetDevice();

	// Heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = count_;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;
		
		device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf()));
		dsvIncSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	// DSVs (One for each array slice)
	dsvCpu_.resize(count_);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(dsvHeap_->GetCPUDescriptorHandleForHeapStart());

	for (uint32_t i = 0; i < count_; ++i)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = kDSVFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.FirstArraySlice = i;
		dsvDesc.Texture2DArray.ArraySize = 1; 
		dsvDesc.Texture2DArray.MipSlice = 0;
		// DSV はスライス単体を指すように作る（描画時便利）
		// あるいは Texture2DArrayとして指定してGSで振り分ける手もあるが、
		// 今回はループで1つずつ描画するスタイル想定

		device->CreateDepthStencilView(depth_.Get(), &dsvDesc, handle);
		dsvCpu_[i] = handle;
		handle.Offset(1, dsvIncSize_);
	}

	// SRV (Entire Array)
	{
		// SRVは DescriptorAllocator から確保
		srv_ = dx12->GetPersistentDescAllocator()->Allocate(1);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = kSRVFormat;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = count_;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(depth_.Get(), &srvDesc, srv_.cpu);
	}
}

DXGI_FORMAT SpotShadowDepthMap::MakeTypelessDepth_(DXGI_FORMAT dsvFmt)
{
	switch (dsvFmt)
	{
	case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_TYPELESS;
	case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24G8_TYPELESS;
	case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_TYPELESS;
	default: return DXGI_FORMAT_UNKNOWN;
	}
}
