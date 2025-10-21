#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>

namespace Tsumi::DX12 {

using Microsoft::WRL::ComPtr;

/* DX12デバイス */
class GraphicsDevice {

private:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	GraphicsDevice() = default;


public:

	/// <summary>
	/// コンストラクタ
	/// コピー禁止
	/// </summary>
	GraphicsDevice(const GraphicsDevice&) = delete;
	GraphicsDevice& operator=(const GraphicsDevice&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~GraphicsDevice();

	/// <summary>
	/// 生成
	/// </summary>
	static std::unique_ptr<GraphicsDevice> Create(bool enableDebug = true);

	/// <summary>
	/// デバッグ・InfoQueue の追加設定
	/// </summary>
	void SetupInfoQueue();

#pragma region Accessor

	ID3D12Device* GetDevice() const { return device_.Get(); }
	IDXGIFactory7* GetFactory() const { return factory_.Get(); }

#pragma endregion


private:

	/// <summary>
	/// 初期化処理
	/// </summary>
	bool Init(bool enableDebug);


private:
	ComPtr<IDXGIFactory7> factory_;
	ComPtr<ID3D12Device> device_;

	// デバッグ関係
	ComPtr<ID3D12Debug1> debugController_;
	ComPtr<ID3D12InfoQueue> infoQueue_;
};

}
