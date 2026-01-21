#include "LightingDirectionalPSOFactory.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Graphic/Shader/ShaderLibrary.h"
#include "DX12/DX12Manager.h"
#include "Graphic/PSO/PSOUtil.h"
#include <stdexcept>

using namespace Tsumi::Graphic;

void LightingDirectionalPSOFactory::Build(PSOLibrary& lib)
{
	const std::string name = "LightingDirectional";

	if (lib.Has(name))
		return;

	auto device = Tsumi::DX12::DX12Manager::GetInstance()->GetDevice();
	auto shaders = ShaderLibrary::GetInstance();
	auto rootsigs = RootSignatureLibrary::GetInstance();

	auto rootSig = rootsigs->Get("LightingDirectional");
	auto vs = shaders->Get("LightingDirectional", ShaderType::VS);
	auto ps = shaders->Get("LightingDirectional", ShaderType::PS);

	if (!rootSig || !vs || !ps)
		throw std::runtime_error("LightingDirectionalPSOFactory: shader or rootsig not found");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSig;

	// --------------------------------
	// InputLayout : なし（SV_VertexID）
	// --------------------------------
	desc.InputLayout = { nullptr, 0 };

	desc.VS = CD3DX12_SHADER_BYTECODE(vs->GetBufferPointer(), vs->GetBufferSize());
	desc.PS = CD3DX12_SHADER_BYTECODE(ps->GetBufferPointer(), ps->GetBufferSize());

	// --------------------------------
	// Blend : 書き込みのみ（加算しない）
	// ※ 複数ライト化したら加算に変える
	// --------------------------------
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	desc.BlendState = blendDesc;

	// --------------------------------
	// Rasterizer
	// --------------------------------
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	// --------------------------------
	// Depth : 無効（Fullscreen）
	// --------------------------------
	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable = FALSE;
	depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depth.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	desc.DepthStencilState = depth;

	// --------------------------------
	// Render Target
	// --------------------------------
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDR Lighting Result
	desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;

	// --------------------------------
	// Create PSO
	// --------------------------------
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
	if (FAILED(hr))
		throw std::runtime_error("LightingDirectionalPSOFactory: CreateGraphicsPipelineState failed");

	lib.Register(name, pso);
}
