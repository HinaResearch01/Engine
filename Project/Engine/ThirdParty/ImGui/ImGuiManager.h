#pragma once

#include <wrl.h>
#include <d3d12.h>
#include "DX12/Desc/DescriptorUtils.h"

// 前方宣言
namespace tme {
namespace sys {
namespace win {
class Win32Window;
}
}
}

namespace Tsumi::DX12 {
class DX12Manager;
struct DescAlloc;
}

namespace tme::gui{

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
	// imgui用のヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiHeap_;

	tme::sys::win::Win32Window* win_ = nullptr;
	Tsumi::DX12::DX12Manager* dx12Mgr_ = nullptr;
	Tsumi::DX12::DescriptorHandle fontSrv_{};
};

}