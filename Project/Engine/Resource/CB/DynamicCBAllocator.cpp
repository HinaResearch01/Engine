#include "DynamicCBAllocator.h"
#include "DX12/PerFrame/PerFrameResource.h"

using namespace Tsumi::Resource;

void DynamicCBAllocator::Attach(Tsumi::DX12::PerFrameResource* fr)
{
	fr_ = fr;
	mapped_ = fr ? fr->GetMappedPtr() : nullptr;
	baseGpu_ = fr ? fr->GetUploadBufferGPUAddr() : 0;
}
