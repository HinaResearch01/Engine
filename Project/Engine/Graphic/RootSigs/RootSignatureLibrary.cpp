#include "RootSignatureLibrary.h"
#include "DX12/DX12Manager.h"
#include "Utils/Func/UtilFunc.h"
#include <d3dx12.h>
#include <stdexcept>
#include <locale>

using namespace Tsumi::Graphic;
using Microsoft::WRL::ComPtr;

RootSignatureLibrary::RootSignatureLibrary() 
{
    dx12Mgr_ = Tsumi::DX12::DX12Manager::GetInstance();
}

void RootSignatureLibrary::Init()
{
    // よく使うルートシグネチャをここで登録する
    CreateObject3D();
	CreateGBuffer();
	CreateLightingDirectional();
}

void RootSignatureLibrary::Register(const std::string& name, const D3D12_ROOT_SIGNATURE_DESC& desc,
    const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers)
{
    // descをコピーして静的サンプラーを付与可能にする
    D3D12_ROOT_SIGNATURE_DESC descCopy = desc;
    if (!staticSamplers.empty()) {
        descCopy.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
        descCopy.pStaticSamplers = staticSamplers.data();
    }
    else {
        descCopy.NumStaticSamplers = 0;
        descCopy.pStaticSamplers = nullptr;
    }

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;

    // ルートシグネチャをシリアライズ
    HRESULT hr = D3D12SerializeRootSignature(
        &descCopy,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &errorBlob
    );

    // 名前を安全に wstring に変換
    std::wstring wname = Utils::Func::Utf8ToWstring(name);

    if (FAILED(hr)) {
        if (errorBlob) {
            // エラーメッセージも UTF-16 に変換
            std::string err((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
            std::wstring werr = Utils::Func::Utf8ToWstring(err);
            Utils::Logger::Error(
				"RootSignatureLibrary::Register - シリアライズエラー '{}': {}\n",
                wname, werr);
        }
        else {
            Utils::Logger::Error(
				"RootSignatureLibrary::Register - シリアライズに失敗 '{}', hr=0x{:08X}\n",
                wname, static_cast<unsigned>(hr));
        }
        throw std::runtime_error("RootSignature serialize failed: " + name);
    }

    // GPU上にルートシグネチャを作成
    ComPtr<ID3D12RootSignature> rootSig;
    hr = dx12Mgr_->GetDevice()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSig)
    );

    if (FAILED(hr) || !rootSig) {
        Utils::Logger::Error(
			"RootSignatureLibrary::Register - CreateRootSignature に失敗 '{}', hr=0x{:08X}\n",
            wname, static_cast<unsigned>(hr));
        throw std::runtime_error("RootSignature create failed: " + name);
    }

    // 登録処理（スレッドセーフ）
    {
        std::lock_guard<std::mutex> lk(mutex_);
        rootSigs_[name] = rootSig;
    }

    Utils::Logger::Info(
		"RootSignatureLibrary::Register - 登録完了 '{}'\n",
        wname);
}

ID3D12RootSignature* RootSignatureLibrary::Get(const std::string& name) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = rootSigs_.find(name);
    if (it == rootSigs_.end()) return nullptr;
    return it->second.Get();
}

bool RootSignatureLibrary::Has(const std::string& name) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return rootSigs_.find(name) != rootSigs_.end();
}

void RootSignatureLibrary::CreateObject3D()
{
	// -------------------------
	// SRV の定義（ディスクリプタ範囲）
	// -------------------------
	// PS: テクスチャ (SRV) 1個 (t0)
	CD3DX12_DESCRIPTOR_RANGE srvRange(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// -------------------------
	// ルートパラメータ定義
	// -------------------------
	CD3DX12_ROOT_PARAMETER params[4]{};

	// VS: Camera (b0)
	params[0].InitAsConstantBufferView(
		0,
		0, D3D12_SHADER_VISIBILITY_VERTEX);

	// VS: Transform (b1)
	params[1].InitAsConstantBufferView(
		1, // b1
		0, D3D12_SHADER_VISIBILITY_VERTEX);

	// PS: Material (b2)
	params[2].InitAsConstantBufferView(
		2, // b2
		0, D3D12_SHADER_VISIBILITY_PIXEL);

	// PS: Texture Albedo (t0)
	params[3].InitAsDescriptorTable(
		1,
		&srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// -------------------------
	// スタティックサンプラー
	// -------------------------
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

	// s0 : Linear Wrap
	D3D12_STATIC_SAMPLER_DESC linearWrap{};
	linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	linearWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.MipLODBias = 0;
	linearWrap.MaxAnisotropy = 0;
	linearWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	linearWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	linearWrap.MinLOD = 0.0f;
	linearWrap.MaxLOD = D3D12_FLOAT32_MAX;
	linearWrap.ShaderRegister = 0; // s0
	linearWrap.RegisterSpace = 0;
	linearWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	staticSamplers.push_back(linearWrap);

	// s1 : Point Clamp
	D3D12_STATIC_SAMPLER_DESC pointClamp =
		CD3DX12_STATIC_SAMPLER_DESC(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	pointClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	staticSamplers.push_back(pointClamp);

	// -------------------------
	// ルートシグネチャ作成
	// -------------------------
	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = _countof(params);
	desc.pParameters = params;
	desc.NumStaticSamplers =
		static_cast<UINT>(staticSamplers.size());
	desc.pStaticSamplers =
		staticSamplers.empty() ? nullptr : staticSamplers.data();
	desc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Register("Object3D", desc, staticSamplers);
}

void RootSignatureLibrary::CreateGBuffer()
{
	// -------------------------
	// SRV Range: AlbedoTex (t0) 1個
	// -------------------------
	CD3DX12_DESCRIPTOR_RANGE srvRange(
	D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // (count, baseReg=t0, space0)

	// -------------------------
	// Root Parameters
	// -------------------------
	CD3DX12_ROOT_PARAMETER params[4]{};

	// b0 : CameraCB (VS)
	params[0].InitAsConstantBufferView(
		0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	// b1 : ObjectCB (VS)
	params[1].InitAsConstantBufferView(
		1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	// b2 : MaterialCB (PS)
	params[2].InitAsConstantBufferView(
		2, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	// t0 : AlbedoTex (PS)
	params[3].InitAsDescriptorTable(
		1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// -------------------------
	// Static Samplers
	// -------------------------
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

	// s0 : Linear Wrap (PS)
	D3D12_STATIC_SAMPLER_DESC linearWrap{};
	linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	linearWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.MipLODBias = 0;
	linearWrap.MaxAnisotropy = 0;
	linearWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	linearWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	linearWrap.MinLOD = 0.0f;
	linearWrap.MaxLOD = D3D12_FLOAT32_MAX;
	linearWrap.ShaderRegister = 0; // s0
	linearWrap.RegisterSpace = 0;
	linearWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	staticSamplers.push_back(linearWrap);

	// -------------------------
	// Root Signature Desc
	// -------------------------
	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = _countof(params);
	desc.pParameters = params;
	desc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
	desc.pStaticSamplers = staticSamplers.data();
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Register("GBuffer", desc, staticSamplers);
}

void RootSignatureLibrary::CreateLightingDirectional()
{
	// -------------------------
	// SRV Range: GBuffer SRVs (t10..t13) 4個
	// t10 : GBuffer0_Albedo
	// t11 : GBuffer1_NormalWS
	// t12 : GBuffer2_Material
	// t13 : Depth01
	// -------------------------
	CD3DX12_DESCRIPTOR_RANGE gbufferRange(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 10, 0); // (count=4, baseReg=t10, space0)

	// -------------------------
	// Root Parameters
	// -------------------------
	CD3DX12_ROOT_PARAMETER params[3]{};

	// b0 : CameraCB (PS)
	params[0].InitAsConstantBufferView(
		0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	// b3 : DirectionalLightCB (PS)
	params[1].InitAsConstantBufferView(
		3, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	// SRV Table : t10..t13 (PS)
	params[2].InitAsDescriptorTable(
		1, &gbufferRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// -------------------------
	// Static Samplers
	// -------------------------
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

	// s1 : Point Clamp (PS)
	D3D12_STATIC_SAMPLER_DESC pointClamp =
		CD3DX12_STATIC_SAMPLER_DESC(
		1, // shaderRegister s1
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	pointClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	staticSamplers.push_back(pointClamp);

	// -------------------------
	// Root Signature Desc
	// -------------------------
	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = _countof(params);
	desc.pParameters = params;
	desc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
	desc.pStaticSamplers = staticSamplers.data();

	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	Register("LightingDirectional", desc, staticSamplers);
}
