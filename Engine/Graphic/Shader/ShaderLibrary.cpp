#include "ShaderLibrary.h"
#include "DX12/DX12Manager.h"
#include <iostream>

using namespace Tsumi::DX12;
using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

static const char* ShaderTypeToTarget(ShaderType t) {
    switch (t) {
        case ShaderType::VS: return "vs_5_0";
        case ShaderType::PS: return "ps_5_0";
        case ShaderType::GS: return "gs_5_0";
        case ShaderType::HS: return "hs_5_0";
        case ShaderType::DS: return "ds_5_0";
        case ShaderType::CS: return "cs_5_0";
        default: return "ps_5_0";
    }
}

void ShaderLibrary::Init()
{
    shaders_.clear();
    Tsumi::Utils::Info(L"[ShaderLibrary] Initialized");
    // シェーダーの一括読み込み
    CompileAllShader();
}

void ShaderLibrary::CompileAllShader()
{
    // ロード関数
    auto tryCompile = [&](const std::wstring& name, const ShaderLoadModule& desc) {
        try {
            // シェーダーのロード
            Compile(name, desc);
        }
        catch (const std::exception& e) {
            // 標準的な例外（エラーメッセージ付き）
            std::wcerr << L"ShaderLibrary::LoadAllShaders - failed to load shader '"
                << name << L"': " << e.what() << std::endl;
        }
        catch (...) {
            // その他の未知の例外
            std::wcerr << L"ShaderLibrary::LoadAllShaders - failed to load shader '"
                << name << L"': unknown error" << std::endl;
        }
        };


    // -------------------------------
    // パーティクル・デカールなど個別シェーダー群
    // -------------------------------
    {
        // ToDo
    }

    // -------------------------------
    // 通常オブジェクト描画用シェーダー群（2D / 3D / スキニング / スカイボックス）
    // -------------------------------
    {
        // 2Dオブジェクト描画
        ShaderLoadModule m;
        m.sources[ShaderType::VS] = L"Resources/Shaders/Object2d/Object2d.VS.hlsl";
        m.sources[ShaderType::PS] = L"Resources/Shaders/Object2d/Object2d.PS.hlsl";
        tryCompile(L"Object2D", m);
    }
    {
        // 3Dオブジェクト描画
        ShaderLoadModule m;
        m.sources[ShaderType::VS] = L"Resources/Shaders/Object3d/Object3d.VS.hlsl";
        m.sources[ShaderType::PS] = L"Resources/Shaders/Object3d/Object3d.PS.hlsl";
        tryCompile(L"Object3D", m);
    }

    // -------------------------------
    // ポストエフェクト群（共通VS + 個別PS）
    // -------------------------------
    // 共通の頂点シェーダー（全ポストエフェクトで使い回し）
    const std::wstring postVs = 
    L"Resources/Shaders/PostEffect/PostEffect.VS.hlsl";

    // 各ポストエフェクトの名前とPSファイルパスをペアで登録
    const std::vector<std::pair<std::wstring, std::wstring>> postEffects = {
        {L"Absent", L"Resources/Shaders/PostEffect/Absent/Absent.PS.hlsl"},
        {L"BoxFilter", L"Resources/Shaders/PostEffect/BoxFilter/BoxFilter.PS.hlsl"},
        {L"ColorGrading", L"Resources/Shaders/PostEffect/ColorGrading/ColorGrading.PS.hlsl"},
        {L"Dissolve", L"Resources/Shaders/PostEffect/Dissolve/Dissolve.PS.hlsl"},
        {L"GaussianFilter", L"Resources/Shaders/PostEffect/GaussianFilter/GaussianFilter.PS.hlsl"},
        {L"Glitch", L"Resources/Shaders/PostEffect/Glitch/Glitch.PS.hlsl"},
        {L"Grain", L"Resources/Shaders/PostEffect/Grain/Grain.PS.hlsl"},
        {L"GrayScale", L"Resources/Shaders/PostEffect/GrayScale/GrayScale.PS.hlsl"},
        {L"HSV", L"Resources/Shaders/PostEffect/HSV/HSV.PS.hlsl"},
        {L"OutLine", L"Resources/Shaders/PostEffect/OutLine/OutLine.PS.hlsl"},
        {L"RadialBlur", L"Resources/Shaders/PostEffect/RadialBlur/RadialBlur.PS.hlsl"},
        {L"Random", L"Resources/Shaders/PostEffect/Random/Random.PS.hlsl"},
        {L"RetroCRT", L"Resources/Shaders/PostEffect/RetroCRT/RetroCRT.PS.hlsl"},
        {L"SepiaTone", L"Resources/Shaders/PostEffect/SepiaTone/SepiaTone.PS.hlsl"},
        {L"Vignetting", L"Resources/Shaders/PostEffect/Vignetting/Vignetting.PS.hlsl"},
    };
    // 全ポストエフェクトを順次ロード
    for (auto& pe : postEffects) {
        ShaderLoadModule d;
        d.sources[ShaderType::VS] = postVs;
        d.sources[ShaderType::PS] = pe.second;
        tryCompile(pe.first, d);
    }

    Tsumi::Utils::Info(L"[ShaderLibrary] LoadAllShaders - completed (errors were logged per-shader if any)");
}

ShaderBlob ShaderLibrary::Get(const std::wstring& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = shaders_.find(name);
    if (it == shaders_.end()) return ShaderBlob{};
    return it->second; // copy (ComPtr inside is copyable)
}

bool ShaderLibrary::Has(const std::wstring& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return shaders_.find(name) != shaders_.end();
}

HRESULT ShaderLibrary::Compile(const std::wstring& name, const ShaderLoadModule& desc)
{
    if (desc.sources.empty()) {
        throw std::runtime_error("ShaderLibrary::Compile - no sources provided for " + std::string(name.begin(), name.end()));
    }

    ShaderBlob compiled;
    for (auto& kv : desc.sources) {
        ShaderType st = kv.first;
        const std::wstring& pathW = kv.second;
        if (pathW.empty()) continue;

        const char* target = ShaderTypeToTarget(st);
        LPCWSTR filename = pathW.c_str();
        LPCSTR entryPoint = "main";
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            filename,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            target,
            compileFlags,
            0,
            &shaderBlob,
            &errorBlob);

        if (FAILED(hr)) {
            std::string msg;
            if (errorBlob) {
                msg.assign(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()),
                    errorBlob->GetBufferSize());
            }
            else {
                msg = "D3DCompileFromFile failed with hr=" + std::to_string(static_cast<unsigned>(hr));
            }
            // Log and throw so the caller's try/catch can handle it
            std::wcerr << L"ShaderLibrary::Compile - Failed to compile " << pathW << L": " << std::wstring(msg.begin(), msg.end()) << std::endl;
            throw std::runtime_error(msg);
        }

        // store compiled blob
        compiled.blob[st] = shaderBlob;
    }

    // commit into map (thread-safe)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shaders_[name] = std::move(compiled);
    }

    Tsumi::Utils::Info(std::wstring(L"[ShaderLibrary] Compiled shader: ") + name);
    return S_OK;
}

