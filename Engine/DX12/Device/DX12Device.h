#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>

namespace Tsumi::DX12 {

/* DX12デバイス */
class DX12Device {

public:
	/// <summary>
	/// コンストラクタ
	/// コピー禁止
	/// </summary>
	DX12Device() = default;
	DX12Device(const DX12Device&) = delete;
	DX12Device& operator=(const DX12Device&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~DX12Device();

	/// <summary>
	/// 生成
	/// </summary>
	HRESULT Create();

#pragma region Accessor

	ID3D12Device* GetDevice() const { return device_.Get(); }
	IDXGIFactory7* GetFactory() const { return factory_.Get(); }

#pragma endregion

private:
	/// <summary>
	/// デバッグレイヤーの生成
	/// </summary>
	HRESULT CreateDebugLayer();

	/// <summary>
	/// ファクトリーの生成
	/// </summary>
	HRESULT CreateFactoryAndAdapter();

	/// <summary>
	/// デバイスの生成
	/// </summary>
	HRESULT CreateDevice();

	/// <summary>
	/// エラーと警告の抑制
	/// </summary>
	HRESULT DebugErrorInfoQueue();

private:
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
	Microsoft::WRL::ComPtr<IDXGIFactory7> factory_;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
	D3D12_INFO_QUEUE_FILTER filter_{};
};

}
