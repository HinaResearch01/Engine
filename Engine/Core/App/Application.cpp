#include "Application.h"
#include <stdexcept>
#include <memory>
#include <Utils/Logger/UtilsLog.h>
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
    dx12_->Init();
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

        // フレーム開始処理
        HRESULT hr = dx12_->StartFrame();
        if (FAILED(hr)) {
            Utils::Log(std::format(L"Application::Run - StartFrame failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
            break;
        }

        // 更新処理
        gameApp_->OnUpdate();

        // 描画処理
        gameApp_->OnBKSpriteRender(); // 背景画像
        gameApp_->OnEntityRender(); // 3Dオブジェクト
        gameApp_->OnFTSpriteRender(); // 前景画像

        // フレーム終了処理
        hr = dx12_->EndFrame();
        if (FAILED(hr)) {
            Utils::Log(std::format(L"Application::Run - EndFrame failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
            break;
        }

        Sleep(0);
    }

    dx12_->GetCommandContext()->WaitForGpu();

    gameApp_->OnFinalize();
}

void Application::SetGameApp(std::unique_ptr<GameApp> game)
{
	gameApp_ = std::move(game);
}
