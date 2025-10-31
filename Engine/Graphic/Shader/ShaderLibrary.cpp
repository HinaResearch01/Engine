#include "ShaderLibrary.h"
#include "DX12/DX12Manager.h"
#include <iostream>

using namespace Tsumi::DX12;
using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

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

ID3DBlob* ShaderLibrary::Get(const std::wstring& name)
{
    
}

bool ShaderLibrary::Has(const std::wstring& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return shaders_.find(name) != shaders_.end();
}

HRESULT ShaderLibrary::Compile(const std::wstring& name, const ShaderLoadModule& desc)
{

}

