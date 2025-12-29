#pragma once

#include <cstdint>
#include <cstddef>
#include <d3d12.h>
#include "DX12/PerFrame/PerFrameResource.h"

// 前方宣言
namespace Tsumi::DX12 { 
class DX12Manager;
}

namespace Tsumi::Resource {

/*  フレームごとに再利用可能な動的定数バッファアロケータ */
class DynamicCBAllocator {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	DynamicCBAllocator() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~DynamicCBAllocator() = default;

	/// <summary>
	/// 
	/// </summary>
	void Attach(Tsumi::DX12::PerFrameResource* fr);

	/// <summary>
	/// 
	/// </summary>
	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS UploadCB(const T& data)
	{
		if (!fr_) return 0;

		constexpr size_t kAlign = 256;
		const size_t bytes = AlignUp(sizeof(T), kAlign);

		size_t offset = 0;
		if (!fr_->Allocate(bytes, kAlign, offset)) {
			return 0; // ここはログ or assert 推奨
		}

		std::memcpy(mapped_ + offset, &data, sizeof(T));
		return baseGpu_ + offset;
	}

private:
	/// <summary>
	/// バイトアライメント
	/// </summary>
	size_t AlignUp(size_t v, size_t a) { return (v + (a - 1)) & ~(a - 1); }

private:
	Tsumi::DX12::PerFrameResource* fr_ = nullptr;
	uint8_t* mapped_ = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS baseGpu_ = 0;
};

}