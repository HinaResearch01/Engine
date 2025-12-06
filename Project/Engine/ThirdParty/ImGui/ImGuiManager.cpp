#include "ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"
#include "DX12/SwapChain/SwapChain.h"
#include "Utils/Logger/UtilsLog.h"

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

	// Docking 有効化
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // マルチウィンドウも使いたければ

	// フォント読み込み
	std::string fontPath = "Resources/font/komorebi.ttf";
	io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 17.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

	// ImGui 用 SRV ヒープの用意
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = dx12Mgr_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiHeap_));

	if (FAILED(hr)) {
		Utils::Log(L"[ImGui] DescriptorHeap 作成失敗\n");
		return;
	}

	// Win32 + DX12 backend 初期化
	ImGui_ImplWin32_Init(win_->GetHWND());

	ImGui_ImplDX12_Init(
		dx12Mgr_->GetDevice(),
		static_cast<int>(dx12Mgr_->GetBufferCount()),
		dx12Mgr_->GetSwapChain()->GetDesc().Format,
		imguiHeap_.Get(),
		imguiHeap_->GetCPUDescriptorHandleForHeapStart(),
		imguiHeap_->GetGPUDescriptorHandleForHeapStart()
	);

	// フォントのテクスチャを強制的に作成
	unsigned char* pixels = nullptr;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// スタイル適用
	StyleSetup();
}

void ImGuiManager::Finalize()
{
	if (dx12Mgr_ && dx12Mgr_->GetCommandContext()) {
		dx12Mgr_->GetCommandContext()->WaitForGpu();
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	imguiHeap_.Reset();
}

void ImGuiManager::BeginFrame()
{
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::Render()
{
	ImGui::Render();
	ID3D12GraphicsCommandList* cmdList = dx12Mgr_->GetCmdList();

	ID3D12DescriptorHeap* heaps[] = { imguiHeap_.Get() };
	cmdList->SetDescriptorHeaps(1, heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImGuiManager::StyleSetup()
{
    ImGuiIO& io = ImGui::GetIO();

    // Style tweaks (compact)
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 12.0f;
    style.TabRounding = 3.0f;

    // カラー設定 (R, G, B, A: 0.0f - 1.0f)
    // 基本色
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f); // 少し暗いグレー
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.0f); // 非常に暗いグレー（ベースを暗く）
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.13f, 0.98f); // ポップアップも暗く
    style.Colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // ボーダーも少し暗く
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // フレーム/入力欄
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.16f, 1.0f); // 濃い目のグレー（暗く）
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f); // ホバー時
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.0f); // アクティブ時
    // タイトルバー（問題の薄いブルーをグレーに統一する箇所）
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);     // 非アクティブ（暗く）
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);     // アクティブ（フレームActiveと統一）
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.13f, 0.51f);    // 縮小時（暗く）
    // メニュー/ヘッダー（ツリーノードなど）
    style.Colors[ImGuiCol_Header] = ImVec4(0.17f, 0.17f, 0.18f, 1.0f); // 少し暗く
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.31f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.36f, 1.0f);
    // ボタン
    style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.16f, 1.0f); // フレームと同じくらい暗く
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.26f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.36f, 1.0f);
    // スクロールバー
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.0f); // 最も暗く
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
    // タブ
    style.Colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.16f, 1.0f);        // 通常タブ（フレームと同じくらい暗く）
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);        // ホバー時
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);        // 選択中（タイトルバーActiveと統一）
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);        // タブ非フォーカス（タイトルバーBgと統一）
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);        // 非フォーカス選択
    // その他
    style.Colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.40f); // モーダルウィンドウの背景も暗く
}
