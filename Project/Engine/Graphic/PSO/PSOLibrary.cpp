#include "PSOLibrary.h"

#include "Factory/Obj3D/Object3DPSOFactory.h"
#include "Factory/Shadow/ShadowPSOFactory.h"

using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

void PSOLibrary::Init() 
{
    // PSOの生成
	Object3DPSOFactory::Build(*this);
	ShadowPSOFactory::Build(*this);
}

void PSOLibrary::Register(const std::string& name, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (pipelineMap_.contains(name)) return;
	pipelineMap_[name] = pso;
}

ID3D12PipelineState* PSOLibrary::Get(const std::string& name)
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = pipelineMap_.find(name);
	return (it != pipelineMap_.end()) ? it->second.Get() : nullptr;
}

bool PSOLibrary::Has(const std::string& name) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return pipelineMap_.contains(name);
}
