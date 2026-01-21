#include "GBufferPSOFactory.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Graphic/Shader/ShaderLibrary.h"
#include "DX12/DX12Manager.h"
#include "Graphic/PSO/PSOUtil.h"
#include <stdexcept>

using namespace Tsumi::Graphic;

void GBufferPSOFactory::Build(PSOLibrary& lib)
{
	const std::string name = "GBuffer";

	if (lib.Has(name))
		return;

	auto device = Tsumi::DX12::DX12Manager::GetInstance()->GetDevice();
	auto shaders = ShaderLibrary::GetInstance();
	auto rootsigs = RootSignatureLibrary::GetInstance();

	auto rootSig = rootsigs->Get("GBuffer");
	auto vs = shaders->Get("GBuffer", ShaderType::VS);
	auto ps = shaders->Get("GBuffer", ShaderType::PS);

	if (!rootSig || !vs || !ps)
		throw std::runtime_error("GBufferPSOFactory: shader or rootsig not found");

	// --------------------------------
	// Input Layout
	// POSITION / NORMAL / TEXCOORD
	// --------------------------------
	static const D3D12_INPUT_ELEMENT_DESC layout[] = {
		PSOUtil::SetUpInputElementDescs("POSITION"),
		PSOUtil::SetUpInputElementDescs("NORMAL"),
		PSOUtil::SetUpInputElementDescs("TEXCOORD"),
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSig;
	desc.InputLayout = { layout, _countof(layout) };
	desc.VS = CD3DX12_SHADER_BYTECODE(vs->GetBufferPointer(), vs->GetBufferSize());
	desc.PS = CD3DX12_SHADER_BYTECODE(ps->GetBufferPointer(), ps->GetBufferSize());

	// --------------------------------
	// Blend (MRT / No Blend)
	// --------------------------------
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	for (int i = 0; i < 3; ++i)
	{
		blendDesc.RenderTarget[i].BlendEnable = FALSE;
		blendDesc.RenderTarget[i].RenderTargetWriteMask =
			D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	desc.BlendState = blendDesc;

	// --------------------------------
	// Rasterizer
	// --------------------------------
	D3D12_RASTERIZER_DESC rs = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rs.CullMode = D3D12_CULL_MODE_BACK;
	desc.RasterizerState = rs;

	// --------------------------------
	// Depth (Write ON)
	// --------------------------------
	D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DepthStencilState = depth;

	// --------------------------------
	// Render Targets (GBuffer)
	// --------------------------------
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	desc.NumRenderTargets = 3;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;        // Albedo
	desc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;   // NormalWS
	desc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;        // Material

	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;

	// --------------------------------
	// Create PSO
	// --------------------------------
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
	if (FAILED(hr))
		throw std::runtime_error("GBufferPSOFactory: CreateGraphicsPipelineState failed");

	lib.Register(name, pso);
}
