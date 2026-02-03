#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>

namespace Tsumi::DX12 {

/* 定数バッファ等の動的データをアップロードするためのリニアアロケータ */
class FrameUploadArena {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameUploadArena() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameUploadArena() { Finalize(); }

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(ID3D12Device* device, uint32_t sizeInBytes);

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void Begin();

	/// <summary>
	/// 汎用アップロード (GPU仮想アドレスを返す)
	/// </summary>
	D3D12_GPU_VIRTUAL_ADDRESS Upload(const void* data, uint32_t size, uint32_t alignment = 256);

	/// <summary>
	/// 型付きヘルパー
	/// </summary>
	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadConst(const T& data) {
		static_assert(sizeof(T) % 256 == 0, "Constant Buffer size must be 256-byte aligned (or use generic Upload with alignment)");
		return Upload(&data, sizeof(T), 256);
	}

	/// <summary>
	/// パディング付きヘルパー
	/// </summary>
	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadConstAlign(const T& data) {
		return Upload(&data, sizeof(T), 256);
	}

private:
	static uint32_t AlignUp(uint32_t value, uint32_t alignment) {
		return (value + alignment - 1) & ~(alignment - 1);
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
	uint8_t* cpuPtr_ = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpuPtr_ = 0;

	uint32_t capacity_ = 0;
	uint32_t currentOffset_ = 0;
};

}