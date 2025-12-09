#include "Application.h"
#include <stdexcept>
#include <memory>
#include <Utils/Logger/UtilsLog.h>
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Graphic/Shader/ShaderLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/Frame/FrameResource.h"
#include "Resource/CB/FrameCBManager.h"
#include "ThirdParty/ImGui/ImGuiManager.h"

using namespace Tsumi;

Application::Application()
{
    window_ = Win32::Win32Window::GetInstance();
    dx12_ = DX12::DX12Manager::GetInstance();
    shaders_ = Graphic::ShaderLibrary::GetInstance();
    rootsigs_ = Graphic::RootSignatureLibrary::GetInstance();
    psos_ = Graphic::PSOLibrary::GetInstance();
    frameResource_ = Graphic::FrameResource::GetInstance();
    frameCBMgr_ = Resource::FrameCBManager::GetInstance();
    imgui_ = GUI::ImGuiManager::GetInstance();
	gameCtx_ = std::make_unique<GameContext>();
}

Application::~Application()
{
	gameCtx_->Finalize();
    imgui_->Finalize();
    window_->OnFinalize();
    dx12_->OnFinalize();
}

void Application::Init(const Win32::Win32Desc& windowDesc)
{
    window_->CreateMainWindow(windowDesc);
    dx12_->Init();
    shaders_->Init();
    shaders_->CompileAllShader();
    rootsigs_->Init();
    psos_->Init();
    frameResource_->Init();
    imgui_->Init();
}

void Application::Run()
{
    // メインループ
    while (!window_->ShouldClose()) {

        // ------------------ ループ開始フェーズ ------------------
        window_->ProcessMessages();
        dx12_->StartFrame();
        frameCBMgr_->BeginFrame(dx12_->GetFrameSync()->GetFrameIndex());
        imgui_->BeginFrame();

		// ------------------ 初期化フェーズ ------------------
		if(gameCtx_->GetPedingInit())
			gameCtx_->Init();

        // ------------------ 更新フェーズ ------------------
        gameCtx_->Update();

        // ------------------ 描画フェーズ ------------------
        {
            dx12_->PreDraw4PE();

            // 背景スプライト（2D）
			gameCtx_->BKSpriteRender();

            // 3Dオブジェクト
            gameCtx_->ModelRender();

            dx12_->PostDraw4PE();
            dx12_->PreDraw4SC();

            // 前景スプライト
            gameCtx_->FTSpriteRender();

            imgui_->Render(); // GUI
            dx12_->PostDraw4SC();
        }

        // ------------------ ループ終了フェーズ ------------------
        dx12_->EndFrame();
        // CPU側で少しスリープ（100%使用防止）
        ::Sleep(0);
    }

    // ------------------ 終了処理フェーズ ------------------
    dx12_->GetCommandContext()->WaitForGpu(); // GPU完了待ち
    gameCtx_->Finalize();
}
