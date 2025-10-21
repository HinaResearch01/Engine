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

public:

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="desc">appデータ</param>
    Application(const AppDesc& desc);

    /// <summary>
    /// コンストラクタ
    /// </summary>
    ~Application();

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


private:
    HWND hwnd_ = nullptr;
    AppDesc desc_;
    bool isRunning_ = true;
    std::unique_ptr<GameApp> gameApp_;
};
}