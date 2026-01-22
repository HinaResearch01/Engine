#include "RootSignatureDesc.h"

using namespace Tsumi::Graphic;

void RootSignatureDesc::AddCBV(UINT shaderRegister,	D3D12_SHADER_VISIBILITY visibility, UINT space)
{
	CD3DX12_ROOT_PARAMETER p{};
	p.InitAsConstantBufferView(shaderRegister, space, visibility);
	params_.push_back(p);
}

void RootSignatureDesc::AddSRVRange(UINT baseRegister, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT space) 
{
	ranges_.emplace_back(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		count,
		baseRegister,
		space
	);

	CD3DX12_ROOT_PARAMETER p{};
	p.InitAsDescriptorTable(1, &ranges_.back(), visibility);
	params_.push_back(p);
}

void RootSignatureDesc::AddStaticSampler(
	UINT shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address, 
	UINT space, D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_STATIC_SAMPLER_DESC s{};
	s.Filter = filter;
	s.AddressU = address;
	s.AddressV = address;
	s.AddressW = address;
	s.MipLODBias = 0;
	s.MaxAnisotropy = 0;
	s.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	s.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	s.MinLOD = 0;
	s.MaxLOD = D3D12_FLOAT32_MAX;
	s.ShaderRegister = shaderRegister;
	s.RegisterSpace = space;
	s.ShaderVisibility = visibility;

	staticSamplers_.push_back(s);
}

D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc::BuildDesc()
{
	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = static_cast<UINT>(params_.size());
	desc.pParameters = params_.data();
	desc.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
	desc.pStaticSamplers = staticSamplers_.data();
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	return desc;
}
