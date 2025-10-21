#pragma once

#include <string>
#include <Windows.h>
#include <memory>
#include "../GameApp.h"

namespace Tsumi {

struct AppDesc {
    std::wstring windowTitle = L"Engine";
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    HINSTANCE hInstance = nullptr;
};

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
    /// メインループ処理
    /// </summary>
    void Run();

    /// <summary>
    /// ゲームの設定
    /// </summary>
    void SetGameApp(std::unique_ptr<GameApp> game);

#pragma region Accessor

    const AppDesc& GetAppDesc() const {
        return desc_;
    }
    void SetAppDesc(const AppDesc& desc) {
        this->desc_ = desc;
    }

    const HWND& GetHwnd() const {
        return hwnd_;
    }

#pragma endregion


private:

    /// <summary>
    /// 初期化処理
    /// </summary>
    void Init();

    /// <summary>
    /// Windowsのメッセージキュー
    /// </summary>
    void ProcessMessages();

    /// <summary>
    /// ウィンドウプロシージャ
    /// </summary>
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    
public:


private:
    HWND hwnd_ = nullptr;
    AppDesc desc_;
    bool isRunning_ = true;
    std::unique_ptr<GameApp> gameApp_;
};
}