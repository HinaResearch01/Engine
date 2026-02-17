#pragma once

#include <string>
#include <Windows.h>
#include <memory>
#include "../Game/GameContext.h"
#include "Utils/FixFPS/FixFPS.h"
#include "Editor/EngineUI.h"

namespace Tsumi {

// 前方宣言
namespace Win32 {
class Win32Window;
struct Win32Desc;
}
namespace DX12 {
class DX12Manager;
class DescriptorAllocator;
}
namespace Graphic {
class ShaderLibrary;
class RootSignatureLibrary;
class PSOLibrary;
}
namespace Resource {
class ResourceSystem;
}
namespace GUI {
class ImGuiManager;
}

/* ウィンドウ・DirectX初期化・メインループ管理 */
class Application {

private: // シングルトン
    Application();
    ~Application();
    Application(const Application&) = delete;
    const Application& operator=(const Application&) = delete;

public:
    /// <summary>
    /// インスタンスの取得
    /// </summary>
    /// <returns></returns>
    static Application* GetInstance() {
        static Application instance;
        return &instance;
    }

    /// <summary>
    /// 初期化処理
    /// </summary>
    void Init(const Win32::Win32Desc& windowDesc);

    /// <summary>
    /// メインループ処理
    /// </summary>
    void Run();

private:
    bool isRunning_ = true;

    Win32::Win32Window* window_ = nullptr;

    DX12::DX12Manager* dx12_ = nullptr;

    Graphic::ShaderLibrary* shaders_ = nullptr;
    Graphic::RootSignatureLibrary* rootsigs_ = nullptr;
    Graphic::PSOLibrary* psos_ = nullptr;

    Resource::ResourceSystem* resourceMgr_ = nullptr;

    GUI::ImGuiManager* imgui_ = nullptr;

    std::unique_ptr<Framework::GameContext> gameCtx_;
    std::unique_ptr<Utils::FixFPS> fixFPS_;
    std::unique_ptr<Editor::EngineUI> engineUI_;
};
}