#include "PerFrameResource.h"
#include <cassert>

using namespace Tsumi::DX12;

HRESULT PerFrameResource::Init(ID3D12Device* device, size_t uploadSize)
{
	assert(device);
	device_ = device;
	upload_.Init(device, uploadSize);
	return S_OK;
}

void Tsumi::DX12::PerFrameResource::InitDescriptors(GlobalDescriptorHeap& globalHeap, uint32_t frameCount, uint32_t tableBase, uint32_t tableCapPerFrame)
{
	tableAlloc_.Init(globalHeap, frameCount, tableBase, tableCapPerFrame);
	tableBuilder_.Init(tableAlloc_, device_);
}

void PerFrameResource::BeginFrame(uint32_t frameIndex)
{
	//upload_.BeginFrame(frameIndex);
	tableBuilder_.BeginFrame(frameIndex);
}