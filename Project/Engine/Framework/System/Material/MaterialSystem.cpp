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

	for (auto [mc] : world_.View<MaterialComponent>())
	{
		if (!mc.visible) continue;

		MaterialKey key{
			mc.surface,
			mc.albedo,
		};

		auto& resolved = ctx_.cache[key];
		resolved.surface = mc.surface;
		resolved.color = mc.color;
		resolved.uvMat = Math::Func::MAT3x3::BuildUVMatrix(mc.uv);
		resolved.visible = mc.visible;

		resolved.albedo = mc.albedo.empty()
			? nullptr
			: resourceSys_->GetTextureManager()->GetTexture(mc.albedo);
	}
}
