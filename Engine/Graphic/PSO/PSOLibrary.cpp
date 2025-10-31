#include "PSOLibrary.h"
#include "PSOUtil.h"
#include "DX12/DX12Manager.h"
#include "../Shader/ShaderLibrary.h"
#include "../Rootsigs/RootSignatureLibrary.h"
#include <stdexcept>
#include <locale>
#include <codecvt>
#include <d3dx12.h>

using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

PSOLibrary::PSOLibrary()
{
	dx12Mgr_ = Tsumi::DX12::DX12Manager::GetInstance();
    shaders_ = Tsumi::Graphic::ShaderLibrary::GetInstance();
    rootsigs_ = Tsumi::Graphic::RootSignatureLibrary::GetInstance();
}

void PSOLibrary::Init() 
{
    // PSOの生成
    CreateObject3D();
}

void PSOLibrary::Register(const std::wstring& name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
    if (pipelines.contains(name))
        return;

    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = dx12Mgr_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
    if (FAILED(hr))
    {
        Tsumi::Utils::Error(L"[PSO] " + name + L" の作成に失敗");
        return;
    }

    pipelines[name] = pso;
    Tsumi::Utils::Info(L"[PSO] " + name + L" 登録完了");
}

ID3D12PipelineState* PSOLibrary::Get(const std::wstring& name)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = pipelines.find(name);
    if (it == pipelines.end()) return nullptr;
    return it->second.Get();
}


bool PSOLibrary::Has(const std::wstring& name) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return pipelines.find(name) != pipelines.end();
}

void PSOLibrary::CreateObject3D()
{
    if (Has(L"Object3D"))
        return;

    // ========================================================
    // ルートシグネチャの取得
    // ========================================================
    auto rootSig = rootsigs_->Get("Object3D");
    if (!rootSig)
    {
        Tsumi::Utils::Error(L"[PSO] Object3D 用 RootSignature が見つかりません。");
        return;
    }

    // ========================================================
    // Input Layout
    // ========================================================
    // 例: Position, Normal, Texcoord, Tangent
    static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        PSOUtil::SetUpInputElementDescs("POSITION"),
        PSOUtil::SetUpInputElementDescs("TEXCOORD"),
        PSOUtil::SetUpInputElementDescs("NORMAL"),
        PSOUtil::SetUpInputElementDescs("WORLDPOSITION"),
    };

    // ========================================================
    // シェーダの読み込み
    // ========================================================
    auto vs = shaders_->Get(L"Object3D", ShaderType::VS);
    auto ps = shaders_->Get(L"Object3D", ShaderType::PS);

    if (!vs || !ps)
    {
        Tsumi::Utils::Error(L"[PSO] Object3D シェーダのロードに失敗しました。");
        return;
    }

    // ========================================================
    // Blend State
    // ========================================================
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = 
        PSOUtil::SetUpBlendState(BlendMode::None);
    blendDesc.RenderTarget[0] = defaultRenderTargetBlendDesc;

    // ========================================================
    // Rasterizer State
    // ========================================================
    D3D12_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;

    // ========================================================
    // DepthStencil State
    // ========================================================
    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthDesc.StencilEnable = FALSE;

    // ========================================================
    // PSO 設定
    // ========================================================
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig;
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs->GetBufferPointer(), vs->GetBufferSize());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps->GetBufferPointer(), ps->GetBufferSize());
    psoDesc.BlendState = blendDesc;
    psoDesc.RasterizerState = rasterDesc;
    psoDesc.DepthStencilState = depthDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    // ========================================================
    // 登録
    // ========================================================
    Register(L"Object3D", psoDesc);
}