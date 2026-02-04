#include "MaterialSystem.h"
#include "Framework/World/World.h"
#include "Resource/ResourceSystem.h"

using namespace Tsumi::Framework;

MaterialSystem::MaterialSystem(World& world)
	: world_(world)
{
	resourceSys_ = Resource::ResourceSystem::GetInstance();
}

void MaterialSystem::Update(float)
{
	ctx_.Clear();

	ctx_.cache.reserve(world_.GetMaterialsCompView().GetActors().size());

	auto* texMgr = resourceSys_->GetTextureManager();

	for (auto [mc] : world_.View<MaterialComponent>())
	{
		if (!mc.visible) continue;

		MaterialKey key{
			mc.surface,
			mc.albedo,
		};

		auto& r = ctx_.cache[key]; // なければ作る／あれば上書き

		r.cb.color = mc.color;
		r.cb.uvTransform = Math::Func::MAT3x3::BuildUVMatrix(mc.uv);

		r.albedo = mc.albedo.empty()
			? nullptr
			: texMgr->GetTexture(mc.albedo);
	}
}
