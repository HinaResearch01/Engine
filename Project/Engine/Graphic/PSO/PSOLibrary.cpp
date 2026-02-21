#include "PSOLibrary.h"
#include "DX12/DX12Manager.h"
#include "Graphic/Shader/ShaderLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Utils/Func/UtilFunc.h"

using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

PSOLibrary::PSOLibrary()
{
	dx12Mgr_ = Tsumi::DX12::DX12Manager::GetInstance();
	shaderLib_ = ShaderLibrary::GetInstance();
	rootSignsLib_ = RootSignatureLibrary::GetInstance();
}

void PSOLibrary::Init()
{
	// 生成と登録
	CreateGBuffer();
	CreateLightingDirectional();
	CreateLightingPoint();
	CreateLightingSpot();
	CreateShadowCaster();
	CreateDeferredComposite();
	CreateDeferredDebug();
}

void PSOLibrary::Register(const std::string& name, PSODesc& pso)
{
	auto desc = pso.BuildDesc();
	RegisterFromDesc(name, desc);
}

ID3D12PipelineState* PSOLibrary::Get(const std::string& name)
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = psos_.find(name);
	return (it != psos_.end()) ? it->second.Get() : nullptr;
}

bool PSOLibrary::Has(const std::string& name) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return psos_.contains(name);
}

void PSOLibrary::RegisterFromDesc(const std::string& name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = dx12Mgr_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));

	std::wstring wname = Utils::Func::Utf8ToWstring(name);

	if (FAILED(hr) || !pso) {
		Utils::Logger::Error(
			"PSOLibrary::RegisterFromDesc - CreateGraphicsPipelineState failed",
			"name:", wname,
			"hr", static_cast<unsigned>(hr));
		throw std::runtime_error("PSO create failed: " + name);
	}

	{
		std::lock_guard<std::mutex> lk(mutex_);
		psos_[name] = pso;
	}

	Utils::Logger::Info("PSOLibrary::RegisterFromDesc - 登録完了", wname);
}

void PSOLibrary::CreateGBuffer()
{
	PSODesc pso;
	pso.SetRootSignature(rootSignsLib_->Get("DeferredGBuffer"));

	auto vs = shaderLib_->Get("DeferredGBuffer", ShaderType::VS);
	auto ps = shaderLib_->Get("DeferredGBuffer", ShaderType::PS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	// InputLayout
	std::vector<D3D12_INPUT_ELEMENT_DESC> il = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pso.SetInputLayout(il);

	pso.SetBlend(BlendMode::Opaque);

	// --- MRT構成の変更 ---
	DXGI_FORMAT rtvs[3] = {
		DXGI_FORMAT_R8G8B8A8_UNORM,     // Target0: Albedo
		DXGI_FORMAT_R16G16B16A16_FLOAT, // Target1: NormalWS
		DXGI_FORMAT_R8G8B8A8_UNORM      // Target2: Reflectivity
	};
	pso.SetRTVFormats(3, rtvs);
	pso.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);

	pso.EnableDepth(true);
	pso.SetDepthWrite(true);
	pso.SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
	pso.SetCullMode(D3D12_CULL_MODE_BACK);

	Register("DeferredGBuffer", pso);
}

void PSOLibrary::CreateLightingDirectional()
{
	PSODesc pso;
	pso.SetRootSignature(rootSignsLib_->Get("DeferredDirectionalLight"));

	auto vs = shaderLib_->Get("DeferredDirectionalLight", ShaderType::VS);
	auto ps = shaderLib_->Get("DeferredDirectionalLight", ShaderType::PS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	pso.SetBlend(BlendMode::Opaque);

	DXGI_FORMAT rtv = dx12Mgr_->GetBackBufferFormat();
	pso.SetRTVFormats(1, &rtv);

	pso.ClearInputLayout();

	pso.EnableDepth(false);
	pso.SetCullMode(D3D12_CULL_MODE_NONE);

	Register("DeferredDirectionalLight", pso);
}

void PSOLibrary::CreateLightingPoint()
{
	PSODesc pso;
	pso.SetRootSignature(rootSignsLib_->Get("DeferredPointLight"));

	auto vs = shaderLib_->Get("DeferredPointLight", ShaderType::VS);
	auto ps = shaderLib_->Get("DeferredPointLight", ShaderType::PS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	// Use Additive Blending for accumulation
	pso.SetBlend(BlendMode::Additive);

	DXGI_FORMAT rtv = dx12Mgr_->GetBackBufferFormat();
	pso.SetRTVFormats(1, &rtv);

	pso.ClearInputLayout(); // Fullscreen triangle

	// No depth test/write (handled in shader via manual depth read if needed, but here we just draw quad)
	pso.EnableDepth(false); 
	pso.SetCullMode(D3D12_CULL_MODE_NONE);

	Register("DeferredPointLight", pso);
}

void PSOLibrary::CreateLightingSpot()
{
	PSODesc pso;
	pso.SetRootSignature(rootSignsLib_->Get("DeferredSpotLight"));

	auto vs = shaderLib_->Get("DeferredSpotLight", ShaderType::VS);
	auto ps = shaderLib_->Get("DeferredSpotLight", ShaderType::PS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	// Use Additive Blending for accumulation
	pso.SetBlend(BlendMode::Additive);

	DXGI_FORMAT rtv = dx12Mgr_->GetBackBufferFormat();
	pso.SetRTVFormats(1, &rtv);

	pso.ClearInputLayout(); // Fullscreen triangle

	pso.EnableDepth(false);
	pso.SetCullMode(D3D12_CULL_MODE_NONE);

	Register("DeferredSpotLight", pso);
}

void PSOLibrary::CreateShadowCaster()
{
	PSODesc pso;

	pso.SetRootSignature(rootSignsLib_->Get("ShadowCaster"));

	auto vs = shaderLib_->Get("ShadowCaster", ShaderType::VS);
	if (!vs)
		throw std::runtime_error("ShadowCaster VS not found");

	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.ClearPS(); // Depth Only

	std::vector<D3D12_INPUT_ELEMENT_DESC> il = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	pso.SetInputLayout(il);

	pso.SetRTVFormats(0, nullptr);
	pso.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);

	pso.EnableDepth(true);
	pso.SetDepthWrite(true);
	pso.SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

	// Change to Front Culling (Render Backfaces)
	pso.SetCullMode(D3D12_CULL_MODE_FRONT);

	// Disable Hardware Bias (We use manual shader bias)
	pso.SetDepthBias(0); 
	pso.SetSlopeScaledDepthBias(0.0f);
	pso.SetDepthBiasClamp(0.0f);

	Register("ShadowCaster", pso);
}

void PSOLibrary::CreateDeferredComposite()
{
	PSODesc pso;

	pso.SetRootSignature(rootSignsLib_->Get("DeferredComposite"));

	auto vs = shaderLib_->Get("DeferredComposite", ShaderType::VS);
	auto ps = shaderLib_->Get("DeferredComposite", ShaderType::PS);

	if (!vs || !ps)
		throw std::runtime_error("DeferredComposite shader not found");

	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	pso.ClearInputLayout();
	pso.SetBlend(BlendMode::Opaque);

	DXGI_FORMAT rtv = dx12Mgr_->GetBackBufferFormat();
	pso.SetRTVFormats(1, &rtv);

	pso.EnableDepth(false);
	pso.SetCullMode(D3D12_CULL_MODE_NONE);

	Register("DeferredComposite", pso);
}

void PSOLibrary::CreateDeferredDebug()
{
	PSODesc pso;

	pso.SetRootSignature(rootSignsLib_->Get("DeferredDebug"));

	auto vs = shaderLib_->Get("DeferredDebug", ShaderType::VS);
	auto ps = shaderLib_->Get("DeferredDebug", ShaderType::PS);

	if (!vs || !ps)
		throw std::runtime_error("DeferredDebug shader not found");

	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	pso.ClearInputLayout();
	pso.SetBlend(BlendMode::Opaque);

	DXGI_FORMAT rtv = dx12Mgr_->GetBackBufferFormat();
	pso.SetRTVFormats(1, &rtv);

	pso.EnableDepth(false);
	pso.SetCullMode(D3D12_CULL_MODE_NONE);

	Register("DeferredDebug", pso);
}
