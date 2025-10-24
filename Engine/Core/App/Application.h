#pragma once

#include <string>
#include <Windows.h>
#include <memory>
#include "../GameApp.h"

namespace Tsumi {

// 前方宣言
namespace Win32 {
class Win32Window;
struct Win32Desc;
}
namespace DX12 {
class DX12Manager;
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

    /// <summary>
    /// ゲームの設定
    /// </summary>
    void SetGameApp(std::unique_ptr<GameApp> game);

private:
    bool isRunning_ = true;
    std::unique_ptr<GameApp> gameApp_;

    Win32::Win32Window* window_ = nullptr;
    DX12::DX12Manager* dx12_ = nullptr;
};
}