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

    Tsumi::Utils::Info(L"[ShaderLibrary] LoadAllShaders - completed (errors were logged per-shader if any)");
}

ID3DBlob* ShaderLibrary::Get(const std::wstring& name, ShaderType stage) const
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

HRESULT ShaderLibrary::Compile(const std::wstring& name, const ShaderLoadModule& desc)
{
    // シェーダソースが指定されていない場合は例外を投げる
    if (desc.sources.empty()) {
        throw std::runtime_error("ShaderLibrary::Compile - no sources provided for " + Tsumi::Utils::WstringToUtf8(name));
    }

    ShaderBlob compiled;

    // 各ステージ（VS, PS, CSなど）ごとにコンパイル処理を行う
    for (auto& kv : desc.sources) {
        ShaderType st = kv.first;
        const std::wstring& pathW = kv.second;
        if (pathW.empty()) continue;

        // ターゲットとエントリーポイントを決定（エントリーポイントは固定で "main"）
        const char* target = ShaderTypeToTarget(st);
        std::string entry = "main";

        // キャッシュキーを生成して、キャッシュ済みCSOを読み込みを試みる
        std::string key = ShaderUtil::ComputeShaderCacheKey(pathW, target, entry);
        auto cached = ShaderUtil::LoadCSOFromDisk(key);
        if (cached) {
            // キャッシュが存在する場合はそれを使用して次へ
            compiled.blob[st] = cached;
            Tsumi::Utils::Info(std::wstring(L"[ShaderLibrary] キャッシュ利用: ") + name + L" (" + std::wstring(pathW.begin(), pathW.end()) + L")");
            continue;
        }

        // キャッシュがない場合はシェーダをコンパイルする
        LPCWSTR filename = pathW.c_str();
        LPCSTR entryPoint = entry.c_str();
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
        // デバッグビルドではデバッグ情報を付与し最適化を無効化
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        // リリースビルドでは最適化を有効化
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        // シェーダファイルをコンパイル
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

        // コンパイル失敗時の処理
        if (FAILED(hr)) {
            std::string msg;
            if (errorBlob) {
                // エラーメッセージをUTF-8からUTF-16に変換
                msg.assign(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()),
                    errorBlob->GetBufferSize());
                int len = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), (int)msg.size(), nullptr, 0);
                std::wstring wmsg;
                if (len > 0) {
                    wmsg.resize(len);
                    MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), (int)msg.size(), &wmsg[0], len);
                }
                Tsumi::Utils::Error(std::wstring(L"[ShaderLibrary] コンパイルエラー: ") + wmsg);
            }
            else {
                msg = "D3DCompileFromFile failed with hr=" + std::to_string(static_cast<unsigned>(hr));
                Tsumi::Utils::Error(std::wstring(L"[ShaderLibrary] コンパイル失敗: ") + std::wstring(msg.begin(), msg.end()));
            }
            throw std::runtime_error(msg);
        }

        // コンパイル結果を一時バッファに保存
        compiled.blob[st] = shaderBlob;

        // キャッシュに保存（失敗しても無視）
        try {
            bool saved = ShaderUtil::SaveCSOToDisk(key, shaderBlob.Get());
            if (saved) {

                Tsumi::Utils::Info(std::wstring(L"[ShaderLibrary] キャッシュに保存: ") + std::wstring(key.begin(), key.end()));
            }
        }
        catch (...) {
            // キャッシュ保存時の例外は無視
        }
    }

    // コンパイル結果をスレッドセーフにマップへ登録
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shaders_[name] = std::move(compiled);
    }

    Tsumi::Utils::Info(std::wstring(L"[ShaderLibrary] シェーダ登録完了: ") + name);
    return S_OK;
}

