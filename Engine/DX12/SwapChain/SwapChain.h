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
	/// </summary>
	SwapChain() = default;
	// コピー禁止
	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~SwapChain() = default;


	bool Init();

	void Present();

#pragma region Accessor



#pragma endregion 


private:



};

}


