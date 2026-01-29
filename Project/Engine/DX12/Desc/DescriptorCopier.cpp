#include "DescriptorCopier.h"

using namespace Tsumi::DX12;

void DescriptorCopier::CopyMany(
	ID3D12Device* device,
	D3D12_CPU_DESCRIPTOR_HANDLE dstStart,
	UINT dstInc,
	std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> src
) {
	if (!device || dstStart.ptr == 0 || src.empty()) return;

	// CopyDescriptorsSimple は連続 src を前提にしないので 1つずつコピーする
	for (size_t i = 0; i < src.size(); ++i) {
		D3D12_CPU_DESCRIPTOR_HANDLE dst = dstStart;
		dst.ptr += static_cast<SIZE_T>(i) * dstInc;
		device->CopyDescriptorsSimple(1, dst, src[i], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}