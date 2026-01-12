#include "MaterialSystem.h"
#include "Framework/Material/MaterialInstance.h"

using namespace Tsumi::Framework;

void MaterialSystem::RegisterMaterial(const std::string& name, RenderMaterial* mat)
{
	materials_[name] = mat;
}

MaterialInstance* MaterialSystem::Acquire(const std::string& name)
{
	auto it = instances_.find(name);
	if (it != instances_.end())
		return it->second.get();

	auto matIt = materials_.find(name);
	if (matIt == materials_.end())
		return nullptr;

	auto inst = std::unique_ptr<MaterialInstance>(CreateInstance(matIt->second));
	auto* ptr = inst.get();
	instances_[name] = std::move(inst);
	return ptr;
}

MaterialInstance* MaterialSystem::CreateInstance(RenderMaterial* mat)
{
	auto* inst = new MaterialInstance();
	inst->parent = mat;

	// 最小：Albedo だけ保証
	inst->SetTexture("Albedo", TextureManager::White());
	inst->SetTexture("Normal", TextureManager::Normal());

	return inst;
}
