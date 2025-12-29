#include "PerFrameResource.h"
#include "DX12/DX12Manager.h"
#include <stdexcept>
#include <cassert>
#include <d3dx12.h> 

using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;

static size_t AlignUp(size_t v, size_t a) {
	return (v + (a - 1)) & ~(a - 1);
}

PerFrameResource::~PerFrameResource()
{
	if (uploadBuffer_) {
		// Unmap before release (safe to call with null range)
		if (mappedUploadPtr_) {
			CD3DX12_RANGE readRange(0, 0);
			uploadBuffer_->Unmap(0, &readRange);
			mappedUploadPtr_ = nullptr;
		}
		uploadBuffer_.Reset();
	}
}

HRESULT PerFrameResource::Init(ID3D12Device* device, size_t uploadBufferSize)
{
	if (!device) return false;
	if (uploadBufferSize == 0) return false;

	uploadBufferSize_ = uploadBufferSize;
	mappedUploadPtr_ = nullptr;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = uploadBufferSize_;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer_));
	if (FAILED(hr) || !uploadBuffer_) {
		uploadBuffer_.Reset();
		return false;
	}

	// Map once and keep mapped pointer for quick CPU writes.
	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from CPU
	hr = uploadBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&mappedUploadPtr_));
	if (FAILED(hr)) {
		uploadBuffer_.Reset();
		mappedUploadPtr_ = nullptr;
		return false;
	}

	fenceValue_ = 0;
	return true;
}

void PerFrameResource::BeginFrame(uint32_t frameIndex)
{
	frameIndex; // 未使用
	// ここでフレーム内の線形領域をリセット
	currentOffset_ = 0;
}

bool PerFrameResource::ResetForRecord() 
{
	return uploadBuffer_ != nullptr;
}

bool PerFrameResource::Allocate(size_t bytes, size_t alignment, size_t& outOffset)
{
	if (alignment == 0) alignment = 1;
	const size_t aligned = AlignUp(currentOffset_, alignment);

	if (!mappedUploadPtr_ || aligned + bytes > uploadBufferSize_) {
		return false;
	}

	outOffset = aligned;
	currentOffset_ = aligned + bytes;
	return true;

}
