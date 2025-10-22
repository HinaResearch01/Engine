#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>

namespace Tsumi::DX12 {

using Microsoft::WRL::ComPtr;

// 前方宣言
class DX12Manager;

/* スワップチェーン */
class SwapChain {

public:
	/// <summary>
	/// コンストラクタ
	/// コピー禁止
	/// </summary>
	SwapChain(DX12Manager* ptr);
	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~SwapChain() = default;

	/// <summary>
	/// 生成
	/// </summary>
	HRESULT Create();

	/// <summary>
	/// Present
	/// </summary>
	HRESULT Present(UINT syncInterval, UINT flags);

	/// <summary>
	/// 現在のバックバッファインデックスを返す
	/// </summary>
	UINT GetCurrentBackBufferIndex() const;

	/// <summary>
	/// バックバッファリソースを取得する
	/// </summary>
	HRESULT GetBuffer(UINT index, ID3D12Resource** outResource) const;

	/// <summary>
	/// スワップチェーンをリサイズ
	/// </summary>
	HRESULT Resize(UINT width, UINT height);

#pragma region Accessor

	IDXGISwapChain4* const GetSwapChain() { return swapChain_.Get(); }
	const DXGI_SWAP_CHAIN_DESC1& GetDesc() const { return desc_; }

#pragma endregion 


private:
	ComPtr<IDXGISwapChain4> swapChain_;
	DXGI_SWAP_CHAIN_DESC1 desc_{};

	// バッファは最大 3 をサポートする配列にしておく
	ComPtr<ID3D12Resource> backBuffers_[3];

	DX12Manager* dx12Mgr_ = nullptr;
};

}


