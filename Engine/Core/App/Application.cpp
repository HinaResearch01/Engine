#include "Application.h"
#include <stdexcept>
#include <memory>

using namespace Tsumi;

Application::Application(const AppDesc& desc)
	: desc_(desc)
{
	Init();
}

Application::~Application()
{
	if (gameApp_) gameApp_->OnFinalize();
	DestroyWindow(hwnd_);
	UnregisterClass(L"EngineWindowClass", desc_.hInstance);
}

void Application::Run()
{
	if (gameApp_) gameApp_->OnInit();

	MSG msg{};
	while (isRunning_) {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				isRunning_ = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!isRunning_) break;

		if (gameApp_) {
			gameApp_->OnUpdate();
			gameApp_->OnRender();
		}
	}
}

void Application::SetGameApp(std::unique_ptr<GameApp> game)
{
	gameApp_ = std::move(game);
}

void Application::Init()
{
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = desc_.hInstance;
	wc.lpszClassName = L"EngineWindowClass";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);

	hwnd_ = CreateWindowEx(
		0,
		wc.lpszClassName,
		desc_.windowTitle.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		desc_.windowWidth, desc_.windowHeight,
		nullptr, nullptr, desc_.hInstance, nullptr
	);

	if (!hwnd_) {
		throw std::runtime_error("Failed to create window.");
	}

	ShowWindow(hwnd_, SW_SHOW);
	UpdateWindow(hwnd_);
}

void Application::ProcessMessages()
{
	MSG msg = {};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			isRunning_ = false; // 終了指示
			break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT Application::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
}
