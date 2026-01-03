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
    std::wstring wname = Utils::Utf8ToWstring(name);

    if (FAILED(hr)) {
        if (errorBlob) {
            // エラーメッセージも UTF-16 に変換
            std::string err((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
            std::wstring werr = Utils::Utf8ToWstring(err);
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

	// VS: 定数バッファ
	params[0].InitAsConstantBufferView(
		0, // b0 
		0, D3D12_SHADER_VISIBILITY_VERTEX);
	params[1].InitAsConstantBufferView(
		1, // b1
		0, D3D12_SHADER_VISIBILITY_VERTEX);

	// PS: 定数バッファ
	params[2].InitAsConstantBufferView(
		2, // b2
		0, D3D12_SHADER_VISIBILITY_PIXEL);

	// PS: テクスチャ SRV t0
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
