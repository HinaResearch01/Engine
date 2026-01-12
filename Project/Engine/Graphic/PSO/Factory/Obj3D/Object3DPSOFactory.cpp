#include "Object3DPSOFactory.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Graphic/Shader/ShaderLibrary.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::Graphic;

static void BuildObject3D(const std::wstring& name, BlendMode blend, bool depthWrite, PSOLibrary& lib);

void Object3DPSOFactory::Build(PSOLibrary& lib)
{
	BuildObject3D(L"Object3D_Opaque",		BlendMode::None, true, lib);
	BuildObject3D(L"Object3D_Masked",		BlendMode::None, true, lib);
	BuildObject3D(L"Object3D_Translucent",	BlendMode::None, false, lib);
	BuildObject3D(L"Object3D_Additive",		BlendMode::None, false, lib);
}

static void BuildObject3D(const std::wstring& name, BlendMode blend, bool depthWrite, PSOLibrary& lib)
{
	if (lib.Has(name))
		return;

	auto device = Tsumi::DX12::DX12Manager::GetInstance()->GetDevice();
	auto shaders = ShaderLibrary::GetInstance();
	auto rootsigs = RootSignatureLibrary::GetInstance();

	auto rootSig = rootsigs->Get("Object3D");
	auto vs = shaders->Get(L"Object3D", ShaderType::VS);
	auto ps = shaders->Get(L"Object3D", ShaderType::PS);

	if (!rootSig || !vs || !ps)
		throw std::runtime_error("Object3DPSOFactory: shader or rootsig not found");

	static const D3D12_INPUT_ELEMENT_DESC layout[] = {
		PSOUtil::SetUpInputElementDescs("POSITION"),
		PSOUtil::SetUpInputElementDescs("TEXCOORD"),
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = rootSig;
	desc.InputLayout = { layout, _countof(layout) };
	desc.VS = CD3DX12_SHADER_BYTECODE(vs->GetBufferPointer(), vs->GetBufferSize());
	desc.PS = CD3DX12_SHADER_BYTECODE(ps->GetBufferPointer(), ps->GetBufferSize());

	// Blend
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0] = PSOUtil::SetUpBlendState(blend);
	desc.BlendState = blendDesc;

	// Raster
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	// Depth
	D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depth.DepthWriteMask = depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState = depth;

	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
	if (FAILED(hr))
		throw std::runtime_error("Object3DPSOFactory: CreateGraphicsPipelineState failed");

	lib.Register(name, pso);
}
