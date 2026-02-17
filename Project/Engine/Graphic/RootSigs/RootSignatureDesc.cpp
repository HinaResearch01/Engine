#include "RootSignatureDesc.h"

using namespace Tsumi::Graphic;

void RootSignatureDesc::AddCBV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT space)
{
	CD3DX12_ROOT_PARAMETER p{};
	p.InitAsConstantBufferView(shaderRegister, space, visibility);
	params_.push_back(p);
}

static CD3DX12_DESCRIPTOR_RANGE* PushRange(
	std::vector<std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE>>& storage,
	D3D12_DESCRIPTOR_RANGE_TYPE type,
	UINT baseRegister,
	UINT count,
	UINT space
)
{
	auto r = std::make_unique<CD3DX12_DESCRIPTOR_RANGE>();
	r->Init(type, count, baseRegister, space);
	auto* ptr = r.get();
	storage.push_back(std::move(r));
	return ptr;
}

void RootSignatureDesc::AddCBVRange(UINT baseRegister, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT space)
{
	auto* range = PushRange(ownedRanges_, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, baseRegister, count, space);

	CD3DX12_ROOT_PARAMETER p{};
	p.InitAsDescriptorTable(1, range, visibility);
	params_.push_back(p);
}

void RootSignatureDesc::AddSRVRange(UINT baseRegister, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT space)
{
	auto* range = PushRange(ownedRanges_, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, baseRegister, count, space);

	CD3DX12_ROOT_PARAMETER p{};
	p.InitAsDescriptorTable(1, range, visibility);
	params_.push_back(p);
}

void RootSignatureDesc::AddUAVRange(UINT baseRegister, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT space)
{
	auto* range = PushRange(ownedRanges_, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, baseRegister, count, space);

	CD3DX12_ROOT_PARAMETER p{};
	p.InitAsDescriptorTable(1, range, visibility);
	params_.push_back(p);
}

void RootSignatureDesc::AddStaticSampler(UINT shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address,
										 UINT space, D3D12_SHADER_VISIBILITY visibility, D3D12_STATIC_BORDER_COLOR borderColor)
{
	D3D12_STATIC_SAMPLER_DESC s{};
	s.Filter = filter;
	s.AddressU = address;
	s.AddressV = address;
	s.AddressW = address;
	s.MipLODBias = 0.0f;
	s.MaxAnisotropy = 16;
	
	if (filter == D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT ||
		filter == D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR ||
		filter == D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT ||
		filter == D3D12_FILTER_COMPARISON_ANISOTROPIC)
	{
		s.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}
	else
	{
		s.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	}

	s.BorderColor = borderColor;
	s.MinLOD = 0.0f;
	s.MaxLOD = D3D12_FLOAT32_MAX;
	s.ShaderRegister = shaderRegister;
	s.RegisterSpace = space;
	s.ShaderVisibility = visibility;
	staticSamplers_.push_back(s);
}

void RootSignatureDesc::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	flags_ = flags;
}

D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc::BuildDesc()
{
	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = static_cast<UINT>(params_.size());
	desc.pParameters = params_.data();
	desc.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
	desc.pStaticSamplers = staticSamplers_.data();
	desc.Flags = flags_;
	return desc;
}
