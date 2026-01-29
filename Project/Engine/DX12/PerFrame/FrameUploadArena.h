#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <cassert>

namespace Tsumi::DX12 {

/*  */
class FrameUploadArena {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameUploadArena() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameUploadArena() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(ID3D12Device* device, uint32_t sizeInBytes);

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

	/// <summary>
	/// 任意データを書き込む（GPU VA を返す）
	/// </summary>
	D3D12_GPU_VIRTUAL_ADDRESS Upload(const void* data, uint32_t size,
									 uint32_t alignment = 256);

	/// <summary>
	/// CB 専用ヘルパ
	/// </summary>
	template<class T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadCB(const T& data) {
		static_assert(std::is_trivially_copyable_v<T>);
		return Upload(&data, sizeof(T), 256);
	}

private:
	static uint32_t AlignUp(uint32_t v, uint32_t align) {
		return (v + (align - 1)) & ~(align - 1);
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
	uint8_t* mapped_ = nullptr;

	uint32_t capacity_ = 0;
	uint32_t offset_ = 0;
};

}