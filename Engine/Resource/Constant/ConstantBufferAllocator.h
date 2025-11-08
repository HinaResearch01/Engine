#pragma once

#include <string>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>
#include "Utils/DXAlign/DXAlign.h"
#include "Utils/DxException/DxException.h"

namespace Tsumi::DX12 {
class DX12Manager;
}

namespace Tsumi::Resource {

struct CBAllocation {
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0; // CBVに渡すGPUアドレス
	void* cpuPtr = nullptr;                   // 書き込み用CPUポインタ
	UINT size = 0;                            // 実際に確保したサイズ（256アライン後）
};

/* CBV（定数バッファ）管理 */
class ConstantBufferAllocator {

private: // シングルトン
	ConstantBufferAllocator();
	~ConstantBufferAllocator() = default;
	ConstantBufferAllocator(const ConstantBufferAllocator&) = delete;
	const ConstantBufferAllocator& operator=(const ConstantBufferAllocator&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static ConstantBufferAllocator* GetInstance() {
		static ConstantBufferAllocator instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// リセット処理
	/// </summary>
	void Reset();

	/// <summary>
	/// 割り当て
	/// </summary>
	CBAllocation Allocate(size_t size);

private:
	static constexpr size_t DefaultSize = 2 * 1024 * 1024; // 2MB（必要なら調整）

	Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap_;
	uint8_t* cpuPtr_ = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpuBase_ = 0;

	size_t currentOffset_ = 0;
	size_t totalSize_ = 0;

	DX12::DX12Manager* dx12Mgr_;

};

}