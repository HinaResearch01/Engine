#include "ShadowDepthMap.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::Graphic;

void ShadowDepthMap::Init(uint32_t size)
{
	Create(size);
}

void ShadowDepthMap::Resize(uint32_t size)
{
	if (size_ == size && tex_) return;
	Create(size);
}

void ShadowDepthMap::Create(uint32_t size)
{
	assert(dx12Mgr_);

	auto* device = dx12Mgr_->GetDevice();
	assert(device);

	size_ = size;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = size;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = kDSVFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clear{};
	clear.Format = kDSVFormat;
	clear.DepthStencil.Depth = 1.0f;
	clear.DepthStencil.Stencil = 0;

	tex_.Reset();
	HRESULT hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear,
		IID_PPV_ARGS(&tex_)
	);
	assert(SUCCEEDED(hr) && tex_);
}
