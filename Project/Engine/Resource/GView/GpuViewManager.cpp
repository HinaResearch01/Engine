#include "GpuViewManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/Logger.h"

using namespace Tsumi::Resource;

GpuViewManager::GpuViewManager()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

void GpuViewManager::Init() {}

void GpuViewManager::RegisterTextureSRV(const std::string& name, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	assert(resource);
	assert(dx12Mgr_);

	if (srvs_.contains(name)) {
		Utils::Logger::Warn("GpuViewManager: SRV '{}' already exists\n", name);
		return;
	}

	auto alloc = dx12Mgr_->GetPersistentDescAlloc()->Allocate(1);
	assert(alloc.valid());

	dx12Mgr_->GetDevice()->CreateShaderResourceView(resource, &desc, alloc.cpuHandle);

	ViewEntry entry{};
	entry.resource = resource;
	entry.desc = alloc;

	srvs_[name] = entry;

	Utils::Logger::Info("GpuViewManager: Registered Texture SRV '{}'\n", name);
}

void GpuViewManager::RegisterStructuredBufferSRV(const std::string& name, ID3D12Resource* resource, UINT stride, UINT elementCount)
{
	assert(resource);
	assert(stride > 0);
	assert(elementCount > 0);

	D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Buffer.FirstElement = 0;
	desc.Buffer.NumElements = elementCount;
	desc.Buffer.StructureByteStride = stride;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	RegisterTextureSRV(name, resource, desc);
}

void GpuViewManager::RegiszterUAV(const std::string& name, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
	assert(resource);
	assert(dx12Mgr_);

	if (uavs_.contains(name)) {
		Utils::Logger::Warn("GpuViewManager: UAV '{}' already exists\n", name);
		return;
	}

	auto alloc = dx12Mgr_->GetPersistentDescAlloc()->Allocate(1);
	assert(alloc.valid());

	dx12Mgr_->GetDevice()->CreateUnorderedAccessView(
		resource,
		nullptr, // counter buffer
		&desc,
		alloc.cpuHandle
	);

	ViewEntry entry{};
	entry.resource = resource;
	entry.desc = alloc;

	uavs_[name] = entry;

	Utils::Logger::Info("GpuViewManager: Registered UAV '{}'\n", name);
}

void GpuViewManager::Clear()
{
	srvs_.clear();
	uavs_.clear();
}
