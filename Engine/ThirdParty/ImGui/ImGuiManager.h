#pragma once

// 前方宣言
namespace Tsumi::Win32 {
class Win32Window;
}
namespace Tsumi::DX12 {
class DX12Manager;
}

namespace Tsumi::GUI {

/* デバッグ用GUI表示 */
class ImGuiManager {

private: // シングルトン
	ImGuiManager();
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	const ImGuiManager& operator=(const ImGuiManager&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static ImGuiManager* GetInstance() {
		static ImGuiManager instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// フレーム開始処理
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// GUI描画
	/// </summary>
	void Render();

private:
	/// <summary>
	/// セットアップ
	/// </summary>
	void StyleSetup();

private:
	Win32::Win32Window* win_ = nullptr;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Tsumi::DX12::DescAlloc guiDescAlloc_{};
};

}