#include "MaterialSystem.h"
#include "Framework/World/World.h"
#include "Resource/ResourceSystem.h"

using namespace Tsumi::Framework;
using namespace tme;
using namespace sys;

MaterialSystem::MaterialSystem(World& world)
	: ISystem(world)
{
	resourceSys_ = resource::ResourceSystem::GetInstance();
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

		auto& r = ctx_.cache[key];

		// ===== UV =====
		r.uv.uvTransform =
			math::func::MAT3x3::BuildUVMatrix(mc.uv);

		// ===== PBR Params =====
		r.params.baseColor = mc.baseColor;
		r.params.alpha = mc.alpha;

		r.params.roughness = mc.roughness;
		r.params.metallic = mc.metallic;
		r.params.ao = mc.ao;
		r.params.useAlbedoTex = mc.useAlbedoTex;

		// ===== Texture =====
		r.albedo = mc.albedo.empty()
			? nullptr
			: texMgr->GetTexture(mc.albedo);
	}
}
