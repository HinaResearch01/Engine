#include "ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"
#include "DX12/SwapChain/SwapChain.h"
#include <d3d12.h>

using namespace Tsumi::GUI;

ImGuiManager::ImGuiManager()
{
    win_ = Win32::Win32Window::GetInstance();
    dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

void ImGuiManager::Init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // ★ Docking 有効化
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // マルチウィンドウも使いたければ

    ImGui::StyleColorsDark();

    // Win32 + DX12 backend 初期化
    ImGui_ImplWin32_Init(win_->GetHWND());
    ImGui_ImplDX12_Init(
        dx12Mgr_->GetDevice(),
        dx12Mgr_->GetSwapChain()->GetDesc().BufferCount,
        RTVManager::GetInstance()->GetDesc().Format,
        DirectXManager::GetInstance()->GetSrvDescriptorHeap(),
        DirectXManager::GetInstance()->GetSrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
        DirectXManager::GetInstance()->GetSrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart()
    );
}

void ImGuiManager::Finalize()
{

}

void ImGuiManager::BeginFrame()
{

}

void ImGuiManager::Render()
{

}

void ImGuiManager::StyleSetup()
{
}
