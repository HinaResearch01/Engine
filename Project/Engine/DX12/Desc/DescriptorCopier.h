#pragma once

#include <d3d12.h>
#include <span>

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

/* GPU-visible heap 上の CPU handle に書き込むために使う */
class DescriptorCopier {

public:
	static void CopyMany(
		ID3D12Device* device,
		D3D12_CPU_DESCRIPTOR_HANDLE dstStart,
		UINT dstInc,
		std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> src
	);
};

}
