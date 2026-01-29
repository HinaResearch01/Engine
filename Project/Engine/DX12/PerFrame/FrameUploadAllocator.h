#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <cassert>
#include <cstring>

namespace Tsumi::DX12 {

/*  */
class FrameUploadAllocator {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameUploadAllocator() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameUploadAllocator() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(ID3D12Device* device, uint32_t bytes);

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

	/// <summary>
	/// アップロード
	/// </summary>
	D3D12_GPU_VIRTUAL_ADDRESS Upload(const void* data, uint32_t bytes, uint32_t align = 256);
	template<class T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadCB(const T& data)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		return Upload(&data, (uint32_t)sizeof(T), 256);
	}

private:
	static uint32_t AlignUp(uint32_t v, uint32_t a) { return (v + (a - 1)) & ~(a - 1); }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> upload_;
	uint8_t* mapped_ = nullptr;

	uint32_t size_ = 0;
	uint32_t offset_ = 0;
	D3D12_GPU_VIRTUAL_ADDRESS baseGpu_ = 0;
};

}
