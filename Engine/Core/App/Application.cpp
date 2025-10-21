#include "Application.h"
#include <stdexcept>
#include <memory>
#include "Win32/Win32Window.h"

using namespace Tsumi;

Application::Application()
{
    window_ = Win32::Win32Window::GetInstance();
}

Application::~Application()
{
	if (gameApp_) gameApp_->OnFinalize();
    window_->OnFinalize();
}

void Application::Init(const Win32::Win32Desc& windowDesc)
{
    window_->CreateMainWindow(windowDesc);
}

void Application::Run()
{
    if (!gameApp_) {
        throw std::runtime_error("GameApp is not set. Call SetGameApp() before Run().");
    }

    gameApp_->OnInit();

    // メインループ
    while (!window_->ShouldClose()) {
        // メッセージ処理
        window_->ProcessMessages();

        // ゲーム処理
        gameApp_->OnUpdate();
        gameApp_->OnRender();
    }

    gameApp_->OnFinalize();
}

void Application::SetGameApp(std::unique_ptr<GameApp> game)
{
	gameApp_ = std::move(game);
}
