#include "Win32Window.h"

using namespace Tsumi::Win32;

void Win32Window::CreateMainWindow(const Win32Desc& desc)
{
	desc_ = desc;

	// アスペクト比をクライアント幅/高さで決める
	if (desc_.windowHeight != 0) {
		aspectRatio_ = static_cast<double>(desc_.windowWidth) / static_cast<double>(desc_.windowHeight);
	}

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = desc_.hInstance;
	wc.lpszClassName = L"EngineWindowClass";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// wc.style = CS_HREDRAW | CS_VREDRAW; // 必要なら
	if (!RegisterClass(&wc)) {
		throw std::runtime_error("Failed to register window class.");
	}

	// CreateWindowEx の lpParam に this を渡して、WndProc でインスタンスを関連づける
	// CreateWindowEx にはウィンドウ全体サイズを渡すため、まずクライアントサイズからウィンドウサイズを求める
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD exStyle = 0;
	RECT clientRect = { 0, 0, static_cast<LONG>(desc_.windowWidth), static_cast<LONG>(desc_.windowHeight) };
	AdjustWindowRectEx(&clientRect, style, FALSE, exStyle);
	int windowWidth = clientRect.right - clientRect.left;
	int windowHeight = clientRect.bottom - clientRect.top;

	hwnd_ = CreateWindowEx(
		exStyle,
		wc.lpszClassName,
		desc_.windowTitle.c_str(),
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		nullptr, nullptr, desc_.hInstance, this // lpParam に this を渡す
	);

	if (!hwnd_) {
		UnregisterClass(wc.lpszClassName, desc_.hInstance);
		throw std::runtime_error("Failed to create window.");
	}

	// 表示
	ShowWindow(hwnd_, SW_SHOW);
	UpdateWindow(hwnd_);
}

void Win32Window::ProcessMessages()
{
	MSG msg = {};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			shouldClose_ = true; 
			break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// WM_NCCREATE の時に CreateWindowEx の lpParam (this) をウィンドウのユーザーデータに保存する
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		void* instancePtr = cs->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instancePtr));

		// インスタンスの hwnd_ を設定しておく（便利）
		if (instancePtr) {
			Win32Window* win = reinterpret_cast<Win32Window*>(instancePtr);
			win->hwnd_ = hwnd;
		}
	}

	Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (window) {
		return window->HandleMessage(msg, wParam, lParam);
	}

	// インスタンスがまだない場合はデフォルト処理
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT Win32Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SIZING:
		{
			RECT* prc = reinterpret_cast<RECT*>(lParam);
			// <- 修正: nullptr のときは break ではなく明示的に戻す
			if (!prc) {
				return DefWindowProc(hwnd_, msg, wParam, lParam);
			}

			RECT currentWndRect;
			RECT currentClientRect;
			GetWindowRect(hwnd_, &currentWndRect);
			GetClientRect(hwnd_, &currentClientRect);

			int currentWindowW = currentWndRect.right - currentWndRect.left;
			int currentWindowH = currentWndRect.bottom - currentWndRect.top;
			int currentClientW = currentClientRect.right - currentClientRect.left;
			int currentClientH = currentClientRect.bottom - currentClientRect.top;

			int borderW = currentWindowW - currentClientW;
			int borderH = currentWindowH - currentClientH;

			int newWindowW = prc->right - prc->left;
			int newWindowH = prc->bottom - prc->top;

			int newClientW = newWindowW - borderW;
			int newClientH = newWindowH - borderH;

			if (newClientW < minClientWidth_) newClientW = minClientWidth_;
			if (newClientH < minClientHeight_) newClientH = minClientHeight_;

			int edge = static_cast<int>(wParam);

			int adjustedClientW = newClientW;
			int adjustedClientH = newClientH;

			switch (edge) {
				case WMSZ_LEFT:
				case WMSZ_RIGHT:
				case WMSZ_TOPLEFT:
				case WMSZ_TOPRIGHT:
				case WMSZ_BOTTOMLEFT:
				case WMSZ_BOTTOMRIGHT:
					adjustedClientW = newClientW;
					adjustedClientH = static_cast<int>(std::round(adjustedClientW / aspectRatio_));
					if (adjustedClientH < minClientHeight_) {
						adjustedClientH = minClientHeight_;
						adjustedClientW = static_cast<int>(std::round(adjustedClientH * aspectRatio_));
					}
					break;

				case WMSZ_TOP:
				case WMSZ_BOTTOM:
				default:
					adjustedClientH = newClientH;
					adjustedClientW = static_cast<int>(std::round(adjustedClientH * aspectRatio_));
					if (adjustedClientW < minClientWidth_) {
						adjustedClientW = minClientWidth_;
						adjustedClientH = static_cast<int>(std::round(adjustedClientW / aspectRatio_));
					}
					break;
			}

			int desiredWindowW = adjustedClientW + borderW;
			int desiredWindowH = adjustedClientH + borderH;

			switch (edge) {
				case WMSZ_LEFT:
					prc->left = prc->right - desiredWindowW;
					prc->bottom = prc->top + desiredWindowH;
					break;
				case WMSZ_RIGHT:
					prc->right = prc->left + desiredWindowW;
					prc->bottom = prc->top + desiredWindowH;
					break;
				case WMSZ_TOP:
					prc->top = prc->bottom - desiredWindowH;
					prc->right = prc->left + desiredWindowW;
					break;
				case WMSZ_BOTTOM:
					prc->bottom = prc->top + desiredWindowH;
					prc->right = prc->left + desiredWindowW;
					break;
				case WMSZ_TOPLEFT:
					prc->left = prc->right - desiredWindowW;
					prc->top = prc->bottom - desiredWindowH;
					break;
				case WMSZ_TOPRIGHT:
					prc->right = prc->left + desiredWindowW;
					prc->top = prc->bottom - desiredWindowH;
					break;
				case WMSZ_BOTTOMLEFT:
					prc->left = prc->right - desiredWindowW;
					prc->bottom = prc->top + desiredWindowH;
					break;
				case WMSZ_BOTTOMRIGHT:
					prc->right = prc->left + desiredWindowW;
					prc->bottom = prc->top + desiredWindowH;
					break;
				default:
					break;
			}

			return TRUE;
		}

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
			if (info) {
				RECT wndRect;
				RECT cliRect;
				GetWindowRect(hwnd_, &wndRect);
				GetClientRect(hwnd_, &cliRect);

				int borderW = (wndRect.right - wndRect.left) - (cliRect.right - cliRect.left);
				int borderH = (wndRect.bottom - wndRect.top) - (cliRect.bottom - cliRect.top);

				info->ptMinTrackSize.x = minClientWidth_ + borderW;
				info->ptMinTrackSize.y = minClientHeight_ + borderH;
			}
			return 0;
		}

		default:
			return DefWindowProc(hwnd_, msg, wParam, lParam);
	}
}

void Win32Window::OnFinalize()
{
	DestroyWindow(hwnd_);
	UnregisterClass(L"EngineWindowClass", desc_.hInstance);
}