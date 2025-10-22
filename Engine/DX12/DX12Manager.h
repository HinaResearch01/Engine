#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include "Utils/DxException/DxException.h"
#include "Device/DX12Device.h"

namespace Tsumi::DX12 {

class DX12Manager {

private: // シングルトン
	// コンストラクタ、デストラクタ
	DX12Manager();
	~DX12Manager() = default;
	DX12Manager(const DX12Manager&) = delete;
	const DX12Manager& operator=(const DX12Manager&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static DX12Manager* GetInstance() {
		static DX12Manager instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 描画前処理 PostEffect
	/// </summary>
	void PreDraw4PE();

	/// <summary>
	/// 描画後処理 PostEffect
	/// </summary>
	void PostDraw4PE();

	/// <summary>
	/// 描画前処理 SwapChain
	/// </summary>
	void PreDraw4SC();

	/// <summary>
	/// 描画後処理 SwapChain
	/// </summary>
	void PostDraw4SC();


#pragma region Accessor

	ID3D12Device* GetDevice() const { 
		if(dx12Device_) return dx12Device_->GetDevice(); }
	IDXGIFactory7* GetFactory() const { 
		if(dx12Device_) return dx12Device_->GetFactory(); }

#pragma endregion


private:
	std::unique_ptr<DX12Device> dx12Device_;

};

}