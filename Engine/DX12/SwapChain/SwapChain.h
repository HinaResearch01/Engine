#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>

namespace Tsumi::DX12 {

using Microsoft::WRL::ComPtr;

/* スワップチェーン */
class SwapChain {

public:

	/// <summary>
	/// コンストラクタ
	/// コピー禁止
	/// </summary>
	SwapChain() = default;
	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~SwapChain() = default;

	/// <summary>
	/// 生成
	/// </summary>
	void Create();

	/// <summary>
	/// Present
	/// </summary>
	void Present(UINT syncInterval, UINT flags);

#pragma region Accessor



#pragma endregion 


private:

	/// <summary>
	/// 初期化処理
	/// </summary>
	bool Init();


private:
	ComPtr<IDXGISwapChain4> swapChain_;
	DXGI_SWAP_CHAIN_DESC1 desc_{};
	UINT bufferCount_ = 2;

	// バッファは最大 3 をサポートする配列にしておく
	ComPtr<ID3D12Resource> backBuffers_[3];
};

}


