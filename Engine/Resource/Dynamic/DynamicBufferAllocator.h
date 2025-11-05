#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

namespace Tsumi::DX12 {
class DX12Manager;
}

namespace Tsumi::Resource {

/* 動的CBV用 */
class DynamicBufferAllocator {

public:
	struct Allocation {
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0; // CBVに渡すGPUアドレス
		void* cpuPtr = nullptr;                   // 書き込み用CPUポインタ
		UINT size = 0;                            // 実際に確保したサイズ（256アライン後）
	};

private: // シングルトン
	DynamicBufferAllocator();
	~DynamicBufferAllocator();
	DynamicBufferAllocator(const DynamicBufferAllocator&) = delete;
	const DynamicBufferAllocator& operator=(const DynamicBufferAllocator&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static DynamicBufferAllocator* GetInstance() {
		static DynamicBufferAllocator instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	HRESULT Init();

	/// <summary>
	/// フレーム開始時に呼ぶ
	/// </summary>
	void Reset();

	/// <summary>
	/// 任意サイズのcbv領域を割り当てる
	/// </summary>
	Allocation Allocate(size_t size);

#pragma region Accessor
	ID3D12Resource* GetResource() const { return buffer_.Get(); }
	size_t GetBufferSize() const { return bufferSize_; }
#pragma endregion

private:
	size_t Align256(size_t size) const {
		return (size + 255) & ~static_cast<size_t>(255);
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
	uint8_t* mappedBegin_ = nullptr;  // Mapした先頭アドレス
	size_t bufferSize_ = 0;
	size_t currentOffset_ = 0;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}

