#include "PSOLibrary.h"

#include "Factory/Obj3D/Object3DPSOFactory.h"

using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

void PSOLibrary::Init() 
{
    // PSOの生成
	Object3DPSOFactory::Build(*this);
}

void PSOLibrary::Register(const std::wstring& name, ID3D12PipelineState* pso)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (pipelineMap_.contains(name)) return;
	pipelineMap_[name] = pso;
}

ID3D12PipelineState* PSOLibrary::Get(const std::wstring& name)
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = pipelineMap_.find(name);
	return (it != pipelineMap_.end()) ? it->second.Get() : nullptr;
}

bool PSOLibrary::Has(const std::wstring& name) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return pipelineMap_.contains(name);
}
