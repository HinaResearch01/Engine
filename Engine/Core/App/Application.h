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
    /// <param name="hwnd">メッセージが送られたウィンドウのハンドル</param>
    /// <param name="msg">メッセージの種類を識別する定数（例: WM_PAINT, WM_DESTROY）</param>
    /// <param name="wParam">メッセージに付随する追加情報（メッセージに依存）</param>
    /// <param name="lParam">メッセージに付随する追加情報（メッセージに依存）</param>
    /// <returns>メッセージ処理の結果。通常はメッセージによって異なる</returns>
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


private:

    HWND hwnd_ = nullptr;
    AppDesc desc_;
    bool isRunning_ = true;
    std::unique_ptr<GameApp> gameApp_;
};
}