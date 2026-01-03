#include "ShaderLibrary.h"
#include "DX12/DX12Manager.h"
#include "ShaderCacheUtil.h"
#include "Utils/Func/UtilFunc.h"
#include <iostream>

using namespace Tsumi::DX12;
using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

static const char* ShaderTypeToTarget(ShaderType t) {
    switch (t) {
        case ShaderType::VS: return "vs_6_0";
        case ShaderType::PS: return "ps_6_0";
        case ShaderType::GS: return "gs_6_0";
        case ShaderType::HS: return "hs_6_0";
        case ShaderType::DS: return "ds_6_0";
        case ShaderType::CS: return "cs_6_0";
        default: return "ps_5_0";
    }
}

void ShaderLibrary::Init()
{
    shaders_.clear();
    Tsumi::Utils::Logger::Info("[ShaderLibrary] Initialized");

    // DXCの初期化
    InitDXC();

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
        // 3Dオブジェクト描画
        ShaderLoadModule m;
        m.sources[ShaderType::VS] = L"Resources/Shaders/VS/Object3D_Static.VS.hlsl";
        m.sources[ShaderType::PS] = L"Resources/Shaders/PS/Object3D_Static.PS.hlsl";
        tryCompile(L"Object3D", m);
    }

    // -------------------------------
    // ポストエフェクト群（共通VS + 個別PS）
    // -------------------------------
    // 共通の頂点シェーダー（全ポストエフェクトで使い回し）
    //const std::wstring postVs = 
    //L"Resources/Shaders/PostEffect/PostEffect.VS.hlsl";

    //// 各ポストエフェクトの名前とPSファイルパスをペアで登録
    //const std::vector<std::pair<std::wstring, std::wstring>> postEffects = {
    //    {L"Absent", L"Resources/Shaders/PostEffect/Absent/Absent.PS.hlsl"},
    //    {L"BoxFilter", L"Resources/Shaders/PostEffect/BoxFilter/BoxFilter.PS.hlsl"},
    //    {L"ColorGrading", L"Resources/Shaders/PostEffect/ColorGrading/ColorGrading.PS.hlsl"},
    //    {L"Dissolve", L"Resources/Shaders/PostEffect/Dissolve/Dissolve.PS.hlsl"},
    //    {L"GaussianFilter", L"Resources/Shaders/PostEffect/GaussianFilter/GaussianFilter.PS.hlsl"},
    //    {L"Glitch", L"Resources/Shaders/PostEffect/Glitch/Glitch.PS.hlsl"},
    //    {L"Grain", L"Resources/Shaders/PostEffect/Grain/Grain.PS.hlsl"},
    //    {L"GrayScale", L"Resources/Shaders/PostEffect/GrayScale/GrayScale.PS.hlsl"},
    //    {L"HSV", L"Resources/Shaders/PostEffect/HSV/HSV.PS.hlsl"},
    //    {L"OutLine", L"Resources/Shaders/PostEffect/OutLine/OutLine.PS.hlsl"},
    //    {L"RadialBlur", L"Resources/Shaders/PostEffect/RadialBlur/RadialBlur.PS.hlsl"},
    //    {L"Random", L"Resources/Shaders/PostEffect/Random/Random.PS.hlsl"},
    //    {L"RetroCRT", L"Resources/Shaders/PostEffect/RetroCRT/RetroCRT.PS.hlsl"},
    //    {L"SepiaTone", L"Resources/Shaders/PostEffect/SepiaTone/SepiaTone.PS.hlsl"},
    //    {L"Vignetting", L"Resources/Shaders/PostEffect/Vignetting/Vignetting.PS.hlsl"},
    //};
    //// 全ポストエフェクトを順次ロード
    //for (auto& pe : postEffects) {
    //    ShaderLoadModule d;
    //    d.sources[ShaderType::VS] = postVs;
    //    d.sources[ShaderType::PS] = pe.second;
    //    tryCompile(pe.first, d);
    //}

    Tsumi::Utils::Logger::Info("[ShaderLibrary] LoadAllShaders - completed (errors were logged per-shader if any)");
}

IDxcBlob* ShaderLibrary::Get(const std::wstring& name, ShaderType stage) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = shaders_.find(name);
    if (it == shaders_.end()) return nullptr;
    auto itStage = it->second.blob.find(stage);
    if (itStage == it->second.blob.end()) return nullptr;
    return itStage->second.Get();
}

bool ShaderLibrary::Has(const std::wstring& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return shaders_.find(name) != shaders_.end();
}

HRESULT ShaderLibrary::Compile(const std::wstring& name, const ShaderLoadModule& module)
{
    // シェーダソースが指定されていない場合は例外を投げる
    if (module.sources.empty()) {
        throw std::runtime_error("ShaderLibrary::Compile - no sources provided for " + Tsumi::Utils::WstringToUtf8(name));
    }
    ShaderBlob compiled;
    HRESULT hr = S_OK;

    for (auto& kv : module.sources) {

        ShaderType stage = kv.first;
        const std::wstring& filePath = kv.second;

        // -------------------------------
        // hlslファイルを読む
        // -------------------------------
        Tsumi::Utils::Logger::Info(
			"[ShaderLibrary] Begin CompileShader, path:{}, stage:{}\n", 
			filePath, static_cast<int>(stage));

        Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
        hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
        if (FAILED(hr) || !shaderSource) {
            Tsumi::Utils::Logger::Error(
				"[ShaderLibrary] Failed to load shader file: {}\n", filePath);
            return hr;
        }

        DxcBuffer shaderSourceBuffer{};
        shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
        shaderSourceBuffer.Size = shaderSource->GetBufferSize();
        shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8文字コード

        // -------------------------------
        // Compilerする
        // -------------------------------
        std::wstring profile;
        switch (stage)
        {
            case ShaderType::VS: profile = L"vs_6_0"; break;
            case ShaderType::PS: profile = L"ps_6_0"; break;
            case ShaderType::GS: profile = L"gs_6_0"; break;
            case ShaderType::HS: profile = L"hs_6_0"; break;
            case ShaderType::DS: profile = L"ds_6_0"; break;
            case ShaderType::CS: profile = L"cs_6_0"; break;
            default: profile = L"ps_6_0"; break;
        }

        LPCWSTR arguments[] = {
            filePath.c_str(),
            L"-E", L"main",          // エントリーポイント
            L"-T", profile.c_str(),  // ターゲットプロファイル
            L"-Zi", L"-Qembed_debug", // デバッグ情報埋め込み
            L"-Od",                  // 最適化無効（開発時）
            L"-Zpr"                  // パッキングルール: 16byte境界
        };

        Microsoft::WRL::ComPtr<IDxcResult> shaderResult;
        hr = dxcCompiler_->Compile(
            &shaderSourceBuffer,
            arguments,
            _countof(arguments),
            dxcIncludeHandler_.Get(),
            IID_PPV_ARGS(&shaderResult)
        );
        if (FAILED(hr) || !shaderResult) {
            Tsumi::Utils::Logger::Error(
				"[ShaderLibrary] DXC Compile failed: {} stage:{}\n", 
				filePath, static_cast<int>(stage));
            return hr;
        }

        // -------------------------------
        // 警告・エラーが出てないか確認する
        // -------------------------------
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError;
        shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

        if (shaderError && shaderError->GetStringLength() != 0) {
            Tsumi::Utils::Logger::Error(
				"[ShaderLibrary] DXC error/warning ({}):\n{}",
                filePath, Tsumi::Utils::Utf8ToWstring(shaderError->GetStringPointer()));
            return E_FAIL; // エラー扱いで中断
        }

        // -------------------------------
        // Compiler結果を受け取って返す
        // -------------------------------
        Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
        hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
        if (FAILED(hr) || !shaderBlob) {
            Tsumi::Utils::Logger::Error(
				"[ShaderLibrary] DXC: failed to get compiled object for {}\n", filePath);
            return hr;
        }

        Tsumi::Utils::Logger::Info(
			"[ShaderLibrary] Compile Succeeded, path:{}, profile:{}\n", 
			filePath, profile);

        compiled.blob[stage] = shaderBlob; // ステージごとに格納
    }

    // スレッドセーフに登録
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shaders_[name] = std::move(compiled);
    }

    return S_OK;
}

HRESULT Tsumi::Graphic::ShaderLibrary::InitDXC()
{
    HRESULT hr{};

    // ComPtrで持つことを推奨
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
    if (FAILED(hr)) return hr;

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
    if (FAILED(hr)) return hr;

    hr = dxcUtils_->CreateDefaultIncludeHandler(&dxcIncludeHandler_);
    if (FAILED(hr)) return hr;

    return hr;
}