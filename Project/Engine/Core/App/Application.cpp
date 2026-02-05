#include "Application.h"
#include <stdexcept>
#include <memory>
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"
#include "Graphic/Shader/ShaderLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Resource/ResourceSystem.h"
#include "ThirdParty/ImGui/ImGuiManager.h"
#include "Utils/Logger/Logger.h"
#include "Utils/Logger/DebugOutputSink.h"
#include "Utils/Logger/FileSink.h"

using namespace Tsumi;

Application::Application()
{
	Utils::Logger::AddSink(std::make_unique<Utils::DebugOutputSink>());
	Utils::Logger::AddSink(std::make_unique<Utils::FileSink>("TsumiEngine.log"));
    window_ = Win32::Win32Window::GetInstance();
    dx12_ = DX12::DX12Manager::GetInstance();
    shaders_ = Graphic::ShaderLibrary::GetInstance();
    rootsigs_ = Graphic::RootSignatureLibrary::GetInstance();
    psos_ = Graphic::PSOLibrary::GetInstance();
    resourceMgr_ = Resource::ResourceSystem::GetInstance();
    imgui_ = GUI::ImGuiManager::GetInstance();
	gameCtx_ = std::make_unique<Framework::GameContext>();
}

Application::~Application()
{
	dx12_->GetCommandContext()->WaitForGpu();
	gameCtx_->Finalize();
    imgui_->Finalize();
	resourceMgr_->Finalize();
    window_->OnFinalize();
    dx12_->Finalize();
}

void Application::Init(const Win32::Win32Desc& windowDesc)
{
    window_->CreateMainWindow(windowDesc);
    dx12_->Init();
    shaders_->Init();
    shaders_->Init();
    rootsigs_->Init();
    psos_->Init();
	resourceMgr_->Init();
    imgui_->Init();
}

void Application::Run()
{
	// メインループ
	while (!window_->ShouldClose()) {

		// =====================================================
		// ループ開始フェーズ
		// =====================================================
		window_->ProcessMessages();

		// ------------------
		// 初期化フェーズ
		// ------------------
		if (gameCtx_->GetPedingInit()) {
			gameCtx_->Init();
		}

		// ------------------
		// 更新フェーズ
		// ------------------
		gameCtx_->Update();

		// =====================================================
		// 描画フェーズ
		// =====================================================
		imgui_->BeginFrame();

		gameCtx_->Render([this]() {
			imgui_->Render();
		});

		// CPU側で少しスリープ（100%使用防止）
		::Sleep(0);
	}

	// =====================================================
	// 終了処理フェーズ
	// =====================================================
	dx12_->WaitForGpu(); // GPU完了待ち
	gameCtx_->Finalize();
}