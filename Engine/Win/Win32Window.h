#pragma once

#include <string>
#include <Windows.h>
#include <memory>
#include <stdexcept>
#include <cmath>

namespace Tsumi::Win32 {

struct Win32Desc {
    std::wstring windowTitle = L"Engine";
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    HINSTANCE hInstance = nullptr;
};

class Win32Window {

private: // シングルトン
    // コンストラクタ、デストラクタ
    Win32Window() = default;
    ~Win32Window() = default;
    Win32Window(const Win32Window&) = delete;
    const Win32Window& operator=(const Win32Window&) = delete;

public:
    /// <summary>
    /// インスタンスの取得
    /// </summary>
    static Win32Window* GetInstance() {
        static Win32Window instance;
        return &instance;
    }

    /// <summary>
    /// ウィンドウの生成
    /// </summary>
    void CreateMainWindow(const Win32Desc& desc);

    /// <summary>
    /// Windowsのメッセージキュー
    /// </summary>
    void ProcessMessages();

    /// <summary>
    /// 開放処理
    /// </summary>
    void OnFinalize();

#pragma region Accessor

    bool ShouldClose() const { return shouldClose_; }
    const HWND& GetHWND() const { return hwnd_; }
    const Win32Desc& GetDesc() const { return desc_; }
    void SetDesc(const Win32Desc& desc) { desc_ = desc; }

#pragma endregion


private:
    /// <summary>
    /// ウィンドウプロシージャ
    /// </summary>
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd_ = nullptr;
    Win32Desc desc_;
    bool shouldClose_ = false;

    // アスペクト比をクライアント領域に対して維持するための値
    double aspectRatio_ = 16.0 / 9.0;

    // 最小クライアントサイズ（必要なら）
    int minClientWidth_ = 200;
    int minClientHeight_ = 200;
};

}

