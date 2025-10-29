#include "ShaderLibrary.h"
#include "ShaderCompiler.h"
#include "Utils/Logger/UtilsLog.h"
#include <stdexcept>
#include <format>
#include <iostream>

using namespace Tsumi::Graphic;

ShaderProgram* ShaderLibrary::LoadOrGet(const std::string& key, const ShaderProgramDesc& desc)
{
    auto it = map_.find(key);
    if (it != map_.end()) return it->second.get();

    auto prog = std::make_unique<ShaderProgram>();
    for (auto& kv : desc.sources) {
        ShaderStage stage = kv.first;
        const std::string& path = kv.second;
        ShaderCompileOptions opt;
        // infer target by stage if not provided
        switch (stage) {
            case ShaderStage::VS: opt.target = "vs_6_0"; break;
            case ShaderStage::PS: opt.target = "ps_6_0"; break;
            case ShaderStage::GS: opt.target = "gs_6_0"; break;
            case ShaderStage::HS: opt.target = "hs_6_0"; break;
            case ShaderStage::DS: opt.target = "ds_6_0"; break;
            case ShaderStage::CS: opt.target = "cs_6_0"; break;
        }
        // Compile & attach (may throw on compile failure)
        prog->CompileAndAttach(stage, path, opt);
    }

    descs_[key] = desc;
    map_[key] = std::move(prog);
    return map_[key].get();
}

ShaderProgram* ShaderLibrary::Get(const std::string& key)
{
    auto it = map_.find(key);
    if (it == map_.end()) return nullptr;
    return it->second.get();
}

bool ShaderLibrary::Reload(const std::string& key)
{
    auto dit = descs_.find(key);
    if (dit == descs_.end()) return false;
    // replace existing program
    ShaderProgramDesc desc = dit->second;
    auto prog = std::make_unique<ShaderProgram>();
    for (auto& kv : desc.sources) {
        ShaderStage stage = kv.first;
        const std::string& path = kv.second;
        ShaderCompileOptions opt;
        switch (stage) {
            case ShaderStage::VS: opt.target = "vs_6_0"; break;
            case ShaderStage::PS: opt.target = "ps_6_0"; break;
            case ShaderStage::GS: opt.target = "gs_6_0"; break;
            case ShaderStage::HS: opt.target = "hs_6_0"; break;
            case ShaderStage::DS: opt.target = "ds_6_0"; break;
            case ShaderStage::CS: opt.target = "cs_6_0"; break;
        }
        prog->CompileAndAttach(stage, path, opt);
    }
    map_[key] = std::move(prog);
    return true;
}

void ShaderLibrary::LoadAllShaders()
{
    // 安全なロード関数（例外が発生しても続行可能にする）
    auto tryLoad = [&](const std::string& name, const ShaderProgramDesc& d) {
        try {
            // シェーダーをロード（既に存在する場合はキャッシュから取得）
            LoadOrGet(name, d);
        }
        catch (const std::exception& e) {
            // 標準的な例外（エラーメッセージ付き）
            std::cerr << "ShaderLibrary::LoadAllShaders - failed to load shader '"
                << name << "': " << e.what() << std::endl;
        }
        catch (...) {
            // その他の未知の例外
            std::cerr << "ShaderLibrary::LoadAllShaders - failed to load shader '"
                << name << "': unknown error" << std::endl;
        }
        };

    // -------------------------------
    // パーティクル・デカールなど個別シェーダー群
    // -------------------------------

    {
        // CPUパーティクル描画用シェーダー
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = "Resources/Shaders/CPUParticle/CPUParticle.VS.hlsl";
        d.sources[ShaderStage::PS] = "Resources/Shaders/CPUParticle/CPUParticle.PS.hlsl";
        tryLoad("CPUParticle", d);
    }
    {
        // デカール（貼り付け型エフェクト）描画用シェーダー
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = "Resources/Shaders/Decal/Decal.VS.hlsl";
        d.sources[ShaderStage::PS] = "Resources/Shaders/Decal/Decal.PS.hlsl";
        tryLoad("Decal", d);
    }
    {
        // 球状エミッター（パーティクル発生源）計算用ComputeShader
        ShaderProgramDesc d;
        d.sources[ShaderStage::CS] = "Resources/Shaders/Emitter/Sphere/SphereEmitter.CS.hlsl";
        tryLoad("Emitter_Sphere", d);
    }
    {
        // 一定方向の力場（Field）処理用ComputeShader
        ShaderProgramDesc d;
        d.sources[ShaderStage::CS] = "Resources/Shaders/Field/Constant/ConstantField.CS.hlsl";
        tryLoad("Field_Constant", d);
    }
    {
        // GPUパーティクル描画用（Drawパス）
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = "Resources/Shaders/GPUParticle/Draw/GPUParticle_Draw.VS.hlsl";
        d.sources[ShaderStage::PS] = "Resources/Shaders/GPUParticle/Draw/GPUParticle_Draw.PS.hlsl";
        tryLoad("GPUParticle_Draw", d);
    }
    {
        // GPUパーティクル初期化用ComputeShader
        ShaderProgramDesc d;
        d.sources[ShaderStage::CS] = "Resources/Shaders/GPUParticle/Init/GPUParticle_Init.CS.hlsl";
        tryLoad("GPUParticle_Init", d);
    }
    {
        // GPUパーティクル更新用ComputeShader
        ShaderProgramDesc d;
        d.sources[ShaderStage::CS] = "Resources/Shaders/GPUParticle/Update/GPUParticle_Update.CS.hlsl";
        tryLoad("GPUParticle_Update", d);
    }

    // -------------------------------
    // 通常オブジェクト描画用シェーダー群（2D / 3D / スキニング / スカイボックス）
    // -------------------------------

    {
        // 2Dオブジェクト描画
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = "Resources/Shaders/Object2d/Object2d.VS.hlsl";
        d.sources[ShaderStage::PS] = "Resources/Shaders/Object2d/Object2d.PS.hlsl";
        tryLoad("Object2D", d);
    }
    {
        // 3Dオブジェクト描画
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = "Resources/Shaders/Object3d/Object3d.VS.hlsl";
        d.sources[ShaderStage::PS] = "Resources/Shaders/Object3d/Object3d.PS.hlsl";
        tryLoad("Object3D", d);
    }
    {
        // スキニング（ボーンアニメーション）対応オブジェクト描画
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = "Resources/Shaders/Skinning/SkinningObject3d.VS.hlsl";
        d.sources[ShaderStage::PS] = "Resources/Shaders/Skinning/SkinningObject3d.PS.hlsl";
        tryLoad("SkinningObject3D", d);
    }
    {
        // スカイボックス描画
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = "Resources/Shaders/Skybox/Skybox.VS.hlsl";
        d.sources[ShaderStage::PS] = "Resources/Shaders/Skybox/Skybox.PS.hlsl";
        tryLoad("Skybox", d);
    }

    // -------------------------------
    // ポストエフェクト群（共通VS + 個別PS）
    // -------------------------------

    // 共通の頂点シェーダー（全ポストエフェクトで使い回し）
    const std::string postVs = "Resources/Shaders/PostEffect/PostEffect.VS.hlsl";

    // 各ポストエフェクトの名前とPSファイルパスをペアで登録
    const std::vector<std::pair<std::string, std::string>> postEffects = {
        {"Absent", "Resources/Shaders/PostEffect/Absent/Absent.PS.hlsl"},
        {"BoxFilter", "Resources/Shaders/PostEffect/BoxFilter/BoxFilter.PS.hlsl"},
        {"ColorGrading", "Resources/Shaders/PostEffect/ColorGrading/ColorGrading.PS.hlsl"},
        {"Dissolve", "Resources/Shaders/PostEffect/Dissolve/Dissolve.PS.hlsl"},
        {"GaussianFilter", "Resources/Shaders/PostEffect/GaussianFilter/GaussianFilter.PS.hlsl"},
        {"Glitch", "Resources/Shaders/PostEffect/Glitch/Glitch.PS.hlsl"},
        {"Grain", "Resources/Shaders/PostEffect/Grain/Grain.PS.hlsl"},
        {"GrayScale", "Resources/Shaders/PostEffect/GrayScale/GrayScale.PS.hlsl"},
        {"HSV", "Resources/Shaders/PostEffect/HSV/HSV.PS.hlsl"},
        {"OutLine", "Resources/Shaders/PostEffect/OutLine/OutLine.PS.hlsl"},
        {"RadialBlur", "Resources/Shaders/PostEffect/RadialBlur/RadialBlur.PS.hlsl"},
        {"Random", "Resources/Shaders/PostEffect/Random/Random.PS.hlsl"},
        {"RetroCRT", "Resources/Shaders/PostEffect/RetroCRT/RetroCRT.PS.hlsl"},
        {"SepiaTone", "Resources/Shaders/PostEffect/SepiaTone/SepiaTone.PS.hlsl"},
        {"Vignetting", "Resources/Shaders/PostEffect/Vignetting/Vignetting.PS.hlsl"},
    };

    // 全ポストエフェクトを順次ロード
    for (auto& pe : postEffects) {
        ShaderProgramDesc d;
        d.sources[ShaderStage::VS] = postVs;
        d.sources[ShaderStage::PS] = pe.second;
        tryLoad("PostEffect_" + pe.first, d);
    }

    // 全シェーダーのロード処理が完了（個別の失敗はログに記録済み）
    Tsumi::Utils::Log("ShaderLibrary::LoadAllShaders - completed (errors were logged per-shader if any)\n");
}