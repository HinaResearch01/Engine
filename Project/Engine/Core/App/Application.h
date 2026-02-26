#pragma once

#include <string>
#include <Windows.h>
#include <memory>
#include "../Game/GameContext.h"
#include "Utils/FixFPS/FixFPS.h"
#include "Editor/EngineUI.h"

namespace Tsumi {
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
}

namespace tme {

namespace sys {
namespace win {
class Win32Window;
struct Win32Desc;
}
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
    void Init(const sys::win::Win32Desc& windowDesc);

    /// <summary>
    /// メインループ処理
    /// </summary>
    void Run();

private:
    bool isRunning_ = true;

	tme::sys::win::Win32Window* window_ = nullptr;

	Tsumi::DX12::DX12Manager* dx12_ = nullptr;

	Tsumi::Graphic::ShaderLibrary* shaders_ = nullptr;
	Tsumi::Graphic::RootSignatureLibrary* rootsigs_ = nullptr;
	Tsumi::Graphic::PSOLibrary* psos_ = nullptr;

	Tsumi::Resource::ResourceSystem* resourceMgr_ = nullptr;

	Tsumi::GUI::ImGuiManager* imgui_ = nullptr;

    std::unique_ptr<Tsumi::Framework::GameContext> gameCtx_;
    std::unique_ptr<tme::util::FixFPS> fixFPS_;
    std::unique_ptr<tme::editor::EngineUI> engineUI_;
};
}