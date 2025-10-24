#include "Application.h"
#include <stdexcept>
#include <memory>
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi;

Application::Application()
{
    window_ = Win32::Win32Window::GetInstance();
    dx12_ = DX12::DX12Manager::GetInstance();
}

Application::~Application()
{
	if (gameApp_) gameApp_->OnFinalize();
    window_->OnFinalize();
    dx12_->OnFinalize();
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

        // 更新処理
        gameApp_->OnUpdate();

        // 描画処理
        gameApp_->OnBKSpriteRender(); // 背景画像
        gameApp_->OnEntityRender(); // 3Dオブジェクト
        gameApp_->OnFTSpriteRender(); // 前景画像
    }

    gameApp_->OnFinalize();
}

void Application::SetGameApp(std::unique_ptr<GameApp> game)
{
	gameApp_ = std::move(game);
}
