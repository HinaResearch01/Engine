#include "RootSignatureLibrary.h"
#include "DX12/DX12Manager.h"
#include <d3dx12.h>
#include <stdexcept>

using namespace Tsumi::Graphic;
using Microsoft::WRL::ComPtr;

RootSignatureLibrary::RootSignatureLibrary(DX12::DX12Manager* ptr)
    : dx12Mgr_(ptr)
{
}

void RootSignatureLibrary::Init() 
{
    CreateObject3D();
}

void RootSignatureLibrary::Register(const std::string& name, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        throw std::runtime_error("RootSignature serialize failed: " + name);
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = dx12Mgr_->GetDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
        throw std::runtime_error("RootSignature create failed: " + name);

    rootSigs_[name] = rootSig;
}

void RootSignatureLibrary::CreateObject3D()
{
    // -------------------------
    // CBV / SRV / Sampler 定義
    // -------------------------
    CD3DX12_DESCRIPTOR_RANGE cbvRangeVS(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // VS:b0
    CD3DX12_DESCRIPTOR_RANGE cbvRangePS(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 7, 0); // PS:b0~b6
    CD3DX12_DESCRIPTOR_RANGE srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);   // t0~t3
    CD3DX12_DESCRIPTOR_RANGE samplerRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // s0

    CD3DX12_ROOT_PARAMETER params[4]{};
    params[0].InitAsDescriptorTable(1, &cbvRangeVS, D3D12_SHADER_VISIBILITY_VERTEX);
    params[1].InitAsDescriptorTable(1, &cbvRangePS, D3D12_SHADER_VISIBILITY_PIXEL);
    params[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    params[3].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // -------------------------
    // Root Signature 作成
    // -------------------------
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = _countof(params);
    desc.pParameters = params;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Register("Object3D", desc);
}
