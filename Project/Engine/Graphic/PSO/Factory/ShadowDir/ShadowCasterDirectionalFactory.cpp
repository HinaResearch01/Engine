#include "ShadowCasterDirectionalFactory.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Graphic/Shader/ShaderLibrary.h"
#include "DX12/DX12Manager.h"
#include <cstddef>

using namespace Tsumi::Graphic;

void ShadowCasterDirectionalFactory::Build(PSOLibrary& lib)
{
	const std::string name = "ShadowCaster_Directional";
	if (lib.Has(name))
		return;

	auto device = Tsumi::DX12::DX12Manager::GetInstance()->GetDevice();
	auto shaders = ShaderLibrary::GetInstance();
	auto rootsigs = RootSignatureLibrary::GetInstance();

	auto rootSig = rootsigs->Get("ShadowCaster_Directional");
	auto vs = shaders->Get("ShadowCaster_Directional", ShaderType::VS);
	auto ps = shaders->Get("ShadowCaster_Directional", ShaderType::PS);

	if (!rootSig || !vs || !ps)
		throw std::runtime_error("ShadowCasterDirectionalFactory: shader or rootsig not found");

	// Input Layout（POSITION だけで十分）
	static const D3D12_INPUT_ELEMENT_DESC layout[] = {
		PSOUtil::SetUpInputElementDescs("POSITION"),
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSig;
	desc.InputLayout = { layout, _countof(layout) };
	desc.VS = CD3DX12_SHADER_BYTECODE(vs->GetBufferPointer(), vs->GetBufferSize());
	desc.PS = CD3DX12_SHADER_BYTECODE(ps->GetBufferPointer(), ps->GetBufferSize());

	// =========================
	// Blend
	// =========================
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// =========================
	// Rasterizer
	// =========================
	D3D12_RASTERIZER_DESC rs = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rs.CullMode = D3D12_CULL_MODE_FRONT;   // シャドウでは Front Cull が定番
	rs.DepthBias = 1000;                   // アクネ対策（仮）
	rs.SlopeScaledDepthBias = 1.0f;
	rs.DepthBiasClamp = 0.0f;
	desc.RasterizerState = rs;

	// =========================
	// Depth
	// =========================
	D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DepthStencilState = depth;

	// =========================
	// その他固定
	// =========================
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Shadow Pass なので RT は使わない
	desc.NumRenderTargets = 0;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
	if (FAILED(hr))
		throw std::runtime_error("ShadowCasterDirectionalFactory: CreateGraphicsPipelineState failed");

	lib.Register(name, pso);
}
