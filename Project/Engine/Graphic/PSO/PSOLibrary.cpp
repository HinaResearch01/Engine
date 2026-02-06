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
	CreateObject3D();
	CreateGBuffer();
	CreateLightingDirectional();
	CreateDebugFullScreen();
	CreateShadowCaster();
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

void PSOLibrary::CreateObject3D()
{
	PSODesc pso;

	pso.SetRootSignature(rootSignsLib_->Get("Object3D"));

	auto vs = shaderLib_->Get("Object3D", ShaderType::VS);
	auto ps = shaderLib_->Get("Object3D", ShaderType::PS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	// InputLayout
	std::vector<D3D12_INPUT_ELEMENT_DESC> il = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	pso.SetInputLayout(il);

	// BlendMode
	pso.SetBlend(BlendMode::Opaque);

	// RT/DS
	DXGI_FORMAT rtv = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso.SetRTVFormats(1, &rtv);
	pso.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);

	// Depth on
	pso.EnableDepth(true);
	pso.SetDepthWrite(true);
	pso.SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

	Register("Object3D", pso);
}

void PSOLibrary::CreateGBuffer()
{
	PSODesc pso;
	pso.SetRootSignature(rootSignsLib_->Get("GBuffer"));

	auto vs = shaderLib_->Get("GBuffer", ShaderType::VS);
	auto ps = shaderLib_->Get("GBuffer", ShaderType::PS);
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

	Register("GBuffer", pso);
}

void PSOLibrary::CreateLightingDirectional()
{
	PSODesc pso;
	pso.SetRootSignature(rootSignsLib_->Get("LightingDirectional"));

	auto vs = shaderLib_->Get("LightingDirectional", ShaderType::VS);
	auto ps = shaderLib_->Get("LightingDirectional", ShaderType::PS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	pso.SetBlend(BlendMode::Opaque);

	// 出力先はバックバッファ（またはポストプロセス用バッファ）1枚
	DXGI_FORMAT rtv = dx12Mgr_->GetBackBufferFormat();
	pso.SetRTVFormats(1, &rtv);

	// フルスクリーントライアングルなので、頂点バッファ(InputLayout)は不要
	pso.ClearInputLayout();

	pso.EnableDepth(false);
	pso.SetCullMode(D3D12_CULL_MODE_NONE);

	Register("LightingDirectional", pso);
}

void PSOLibrary::CreateDebugFullScreen()
{
	PSODesc pso;

	pso.SetRootSignature(rootSignsLib_->Get("DebugFullScreen"));

	auto vs = shaderLib_->Get("DebugFullscreen", ShaderType::VS);
	auto ps = shaderLib_->Get("DebugFullscreen", ShaderType::PS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });
	pso.SetPS({ ps->GetBufferPointer(), ps->GetBufferSize() });

	// Fullscreen triangle：InputLayout なし
	pso.ClearInputLayout();

	// BlendMode
	pso.SetBlend(BlendMode::Opaque);

	// BackBuffer へ 1RT
	DXGI_FORMAT rtv = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso.SetRTVFormats(1, &rtv);
	pso.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);

	// Depth off
	pso.EnableDepth(false);

	// Cull none が無難
	pso.SetCullMode(D3D12_CULL_MODE_NONE);

	Register("DebugFullScreen", pso);
}

void PSOLibrary::CreateShadowCaster()
{
	PSODesc pso;

	pso.SetRootSignature(rootSignsLib_->Get("ShadowCaster"));

	auto vs = shaderLib_->Get("ShadowCaster", ShaderType::VS);
	pso.SetVS({ vs->GetBufferPointer(), vs->GetBufferSize() });

	// PS は無し（Depth Only）
	pso.ClearPS();

	// InputLayout
	std::vector<D3D12_INPUT_ELEMENT_DESC> il = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	pso.SetInputLayout(il);

	// RenderTargetなし
	pso.SetRTVFormats(0, nullptr);
	pso.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);

	// Depth ON
	pso.EnableDepth(true);
	pso.SetDepthWrite(true);
	pso.SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

	// Rasterizer
	pso.SetCullMode(D3D12_CULL_MODE_BACK);

	// Depth bias（
	pso.SetDepthBias(1000);
	pso.SetSlopeScaledDepthBias(1.0f);
	pso.SetDepthBiasClamp(0.0f);

	Register("ShadowCaster", pso);
}
