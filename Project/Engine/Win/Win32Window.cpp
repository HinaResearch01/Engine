#include "Win32Window.h"

using namespace Tsumi::Win32;

void Win32Window::CreateMainWindow(const Win32Desc& desc)
{
	desc_ = desc;
	targetAspectRatio_ = static_cast<double>(desc_.windowWidth) / static_cast<double>(desc_.windowHeight);

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = desc_.hInstance;
	wc.lpszClassName = L"EngineWindowClass";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if (!RegisterClass(&wc)) {
		throw std::runtime_error("Failed to register window class.");
	}

	// 1. クライアントサイズからウィンドウ全体のサイズを逆算
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD exStyle = 0;
	RECT clientRect = { 0, 0, static_cast<LONG>(desc_.windowWidth), static_cast<LONG>(desc_.windowHeight) };
	AdjustWindowRectEx(&clientRect, style, FALSE, exStyle);

	int windowWidth = clientRect.right - clientRect.left;
	int windowHeight = clientRect.bottom - clientRect.top;

	// 2. ウィンドウ生成
	hwnd_ = CreateWindowEx(
		exStyle,
		wc.lpszClassName,
		desc_.windowTitle.c_str(),
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		nullptr, nullptr, desc_.hInstance, this
	);

	if (!hwnd_) {
		UnregisterClass(wc.lpszClassName, desc_.hInstance);
		throw std::runtime_error("Failed to create window.");
	}

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
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	if (msg == WM_NCCREATE) {
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		void* instancePtr = cs->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instancePtr));

		if (instancePtr) {
			Win32Window* win = reinterpret_cast<Win32Window*>(instancePtr);
			win->hwnd_ = hwnd;
		}
	}

	Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (window) {
		return window->HandleMessage(msg, wParam, lParam);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT Win32Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		case WM_SIZE:
		// 必要に応じてここでDX12のリソース（SwapChain）再作成フラグを立てる
		return 0;

		case WM_SIZING: {
			RECT* prc = reinterpret_cast<RECT*>(lParam);
			RECT wndR, cliR;
			GetWindowRect(hwnd_, &wndR);
			GetClientRect(hwnd_, &cliR);

			int borderW = (wndR.right - wndR.left) - (cliR.right - cliR.left);
			int borderH = (wndR.bottom - wndR.top) - (cliR.bottom - cliR.top);

			int targetW = prc->right - prc->left - borderW;
			int targetH = prc->bottom - prc->top - borderH;

			// 比率維持ロジック
			if (wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT) {
				targetH = static_cast<int>(targetW / targetAspectRatio_);
			}
			else {
				targetW = static_cast<int>(targetH * targetAspectRatio_);
			}

			// サイズ適用
			int finalW = targetW + borderW;
			int finalH = targetH + borderH;

			if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT) prc->top = prc->bottom - finalH;
			else prc->bottom = prc->top + finalH;

			if (wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT) prc->left = prc->right - finalW;
			else prc->right = prc->left + finalW;

			return TRUE;
		}
	}
	return DefWindowProc(hwnd_, msg, wParam, lParam);
}

void Win32Window::OnFinalize()
{
	DestroyWindow(hwnd_);
	UnregisterClass(L"EngineWindowClass", desc_.hInstance);
}