#pragma once

#include <string>
#include <Windows.h>
#include <memory>
#include <stdexcept>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace tme::sys::Win32 {

struct Win32Desc {
    std::wstring windowTitle = L"Engine";
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    HINSTANCE hInstance = nullptr;
};

/* ウィンドウ管理クラス */
class Win32Window {

private: // シングルトン
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
	uint32_t GetClientWidth() const { 
		RECT r;
		GetClientRect(hwnd_, &r);
		return static_cast<uint32_t>(r.right - r.left);
	}
	uint32_t GetClientHeight() const { 
		RECT r;
		GetClientRect(hwnd_, &r);
		return static_cast<uint32_t>(r.bottom - r.top);
	}
	float GetCurrentAspectRatio() const { 
		uint32_t w = GetClientWidth();
		uint32_t h = GetClientHeight();
		return (h != 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;
	}
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
	double targetAspectRatio_ = 16.0 / 9.0;

    // 最小クライアントサイズ（必要なら）
    int minClientWidth_ = 200;
    int minClientHeight_ = 200;
};

}

