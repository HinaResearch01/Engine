#include "PSODesc.h"
#include <cstring>

using namespace Tsumi::Graphic;

PSODesc::PSODesc()
{
	std::memset(&desc_, 0, sizeof(desc_));

	desc_.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc_.RasterizerState.FrontCounterClockwise = FALSE;
	desc_.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	desc_.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	desc_.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	desc_.SampleMask = UINT_MAX;

	desc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	desc_.SampleDesc.Count = 1;
	desc_.SampleDesc.Quality = 0;


	desc_.NumRenderTargets = 0;
	for (int i = 0; i < 8; ++i) desc_.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	desc_.DSVFormat = DXGI_FORMAT_UNKNOWN;
}

void PSODesc::SetRootSignature(ID3D12RootSignature* rs)
{
	rootSig_ = rs;
}

void PSODesc::SetVS(const D3D12_SHADER_BYTECODE& vs) { vs_ = vs; }
void PSODesc::SetPS(const D3D12_SHADER_BYTECODE& ps) { ps_ = ps; }
void PSODesc::ClearPS() { ps_ = {}; }

void PSODesc::SetRTVFormats(UINT count, const DXGI_FORMAT* fmts)
{
	desc_.NumRenderTargets = count;
	for (UINT i = 0; i < 8; ++i)
		desc_.RTVFormats[i] = (i < count) ? fmts[i] : DXGI_FORMAT_UNKNOWN;
}

void PSODesc::SetDSVFormat(DXGI_FORMAT fmt)
{
	desc_.DSVFormat = fmt;
}

void PSODesc::SetRasterizer(const D3D12_RASTERIZER_DESC& rs) { desc_.RasterizerState = rs; }

void PSODesc::SetBlend(BlendMode mode) 
{ 
	D3D12_BLEND_DESC bd = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	auto& rt = bd.RenderTarget[0];
	rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	switch (mode)
	{
		case BlendMode::Opaque:
		rt.BlendEnable = FALSE;
		break;

		case BlendMode::Alpha:
		rt.BlendEnable = TRUE;
		rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		break;

		case BlendMode::Additive:
		rt.BlendEnable = TRUE;
		rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rt.DestBlend = D3D12_BLEND_ONE;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ONE;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		break;

		case BlendMode::Multiply:
		rt.BlendEnable = TRUE;
		rt.SrcBlend = D3D12_BLEND_DEST_COLOR;
		rt.DestBlend = D3D12_BLEND_ZERO;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		break;
	}

	desc_.BlendState = bd;
}

void PSODesc::SetDepthStencil(const D3D12_DEPTH_STENCIL_DESC& dss) { desc_.DepthStencilState = dss; }

void PSODesc::SetCullMode(D3D12_CULL_MODE cull)
{
	desc_.RasterizerState.CullMode = cull;
}

void PSODesc::EnableDepth(bool enable)
{
	if (enable)
	{
		desc_.DepthStencilState.DepthEnable = TRUE;
		desc_.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc_.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	}
	else
	{
		desc_.DepthStencilState.DepthEnable = FALSE;
		desc_.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		desc_.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}
}

void PSODesc::SetDepthFunc(D3D12_COMPARISON_FUNC func)
{
	desc_.DepthStencilState.DepthFunc = func;
}

void PSODesc::SetDepthWrite(bool enable)
{
	desc_.DepthStencilState.DepthWriteMask =
		enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
}

void PSODesc::SetDepthBias(int32_t bias)
{
	desc_.RasterizerState.DepthBias = bias;
}

void PSODesc::SetSlopeScaledDepthBias(float bias)
{
	desc_.RasterizerState.SlopeScaledDepthBias = bias;
}

void PSODesc::SetDepthBiasClamp(float clamp)
{
	desc_.RasterizerState.DepthBiasClamp = clamp;
}

void PSODesc::SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& elems)
{
	inputElems_ = elems;
	desc_.InputLayout = { inputElems_.data(), (UINT)inputElems_.size() };
}

void PSODesc::ClearInputLayout()
{
	inputElems_.clear();
	desc_.InputLayout = { nullptr, 0 };
}

void PSODesc::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	desc_.PrimitiveTopologyType = type;
}

void PSODesc::SetSampleDesc(UINT count, UINT quality)
{
	desc_.SampleDesc.Count = count;
	desc_.SampleDesc.Quality = quality;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc::BuildDesc() const
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC out = desc_;
	out.pRootSignature = rootSig_;
	out.VS = vs_;
	out.PS = ps_;
	return out;
}
