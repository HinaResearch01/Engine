#include "RootSignatureLibrary.h"
#include "DX12/DX12Manager.h"
#include <d3dx12.h>
#include <stdexcept>
#include <locale>
#include <codecvt>

using namespace Tsumi::Graphic;
using Microsoft::WRL::ComPtr;

RootSignatureLibrary::RootSignatureLibrary(DX12::DX12Manager* ptr)
    : dx12Mgr_(ptr)
{
}

void RootSignatureLibrary::Init()
{
    // よく使うルートシグネチャをここで登録する
    CreateObject3D();
}

void RootSignatureLibrary::Register(const std::string& name, const D3D12_ROOT_SIGNATURE_DESC& desc,
    const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers)
{
    // descをコピーして、静的サンプラーを付与できるようにする
    D3D12_ROOT_SIGNATURE_DESC descCopy = desc;

    // 静的サンプラーが指定されている場合は設定する
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

    // ルートシグネチャをシリアライズしてバイナリ化する
    HRESULT hr = D3D12SerializeRootSignature(&descCopy, D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            // エラーメッセージをwstringに変換してログ出力
            std::string err((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
            std::wstring werr = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(err);
            Utils::Log(std::format(L"RootSignatureLibrary::Register - シリアライズエラー '{}': {}\n", std::wstring(name.begin(), name.end()), werr));
        }
        else {
            Utils::Log(std::format(L"RootSignatureLibrary::Register - シリアライズに失敗 '{}', hr=0x{:08X}\n",
                std::wstring(name.begin(), name.end()), static_cast<unsigned>(hr)));
        }
        throw std::runtime_error("RootSignature serialize failed: " + name);
    }

    // GPU上にルートシグネチャを生成
    ComPtr<ID3D12RootSignature> rootSig;
    hr = dx12Mgr_->GetDevice()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));
    if (FAILED(hr) || !rootSig) {
        Utils::Log(std::format(L"RootSignatureLibrary::Register - CreateRootSignature に失敗 '{}', hr=0x{:08X}\n",
            std::wstring(name.begin(), name.end()), static_cast<unsigned>(hr)));
        throw std::runtime_error("RootSignature create failed: " + name);
    }

    {
        std::lock_guard<std::mutex> lk(mutex_);
        rootSigs_[name] = rootSig;
    }

    Utils::Log(std::format(L"RootSignatureLibrary::Register - 登録完了 '{}'\n", std::wstring(name.begin(), name.end())));
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
    // CBV / SRV の定義（ディスクリプタ範囲）
    // -------------------------
    // VS: 定数バッファ1個 (b0)
    CD3DX12_DESCRIPTOR_RANGE cbvRangeVS(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    // PS: 定数バッファ7個 (b0～b6)
    CD3DX12_DESCRIPTOR_RANGE cbvRangePS(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 7, 0);
    // PS: テクスチャ (SRV) 4個 (t0～t3)
    CD3DX12_DESCRIPTOR_RANGE srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

    // ルートパラメータを定義（VS用CBVテーブル、PS用CBVテーブル、PS用SRVテーブル）
    CD3DX12_ROOT_PARAMETER params[3]{};
    params[0].InitAsDescriptorTable(1, &cbvRangeVS, D3D12_SHADER_VISIBILITY_VERTEX);
    params[1].InitAsDescriptorTable(1, &cbvRangePS, D3D12_SHADER_VISIBILITY_PIXEL);
    params[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // -------------------------
    // スタティックサンプラー定義（共通設定）
    // -------------------------
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

    // s0 : 線形補間＋ラップモード（一般的なテクスチャサンプリング）
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

    // s1 : 最近傍＋クランプモード（UIやピクセルくっきり表現などに使用）
    D3D12_STATIC_SAMPLER_DESC pointClamp = CD3DX12_STATIC_SAMPLER_DESC(
        1, // s1 レジスタ
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    pointClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    staticSamplers.push_back(pointClamp);

    // -------------------------
    // ルートシグネチャの作成
    // -------------------------
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = _countof(params);
    desc.pParameters = params;
    desc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
    desc.pStaticSamplers = staticSamplers.empty() ? nullptr : staticSamplers.data();
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // "Object3D" という名前で登録
    Register("Object3D", desc, staticSamplers);
}
