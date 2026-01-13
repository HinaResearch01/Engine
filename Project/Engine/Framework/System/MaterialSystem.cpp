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
	cache_.clear();

	// MaterialComponent を持つ Actor を View で列挙
	for (auto [mc] : world_.View<MaterialComponent>())
	{
		if (!mc.visible)
			continue;

		MaterialKey key{
			mc.surface,
			mc.albedo,
			mc.normal,
			mc.metallic,
			mc.roughness
		};

		auto& pkt = cache_[key];

		// GPU material CB
		pkt.cb.metallic = mc.metallic;
		pkt.cb.roughness = mc.roughness;
		pkt.cb.baseColor = { 1,1,1,1 };

		// TextureManager から GPU リソースを取得
		pkt.albedo = mc.albedo.empty()
			? nullptr
			: resourceSys_->GetTextureManager()->GetTexture(mc.albedo);

		pkt.normal = mc.normal.empty()
			? nullptr
			: resourceSys_->GetTextureManager()->GetTexture(mc.normal);
	}
}

const MaterialPacket* MaterialSystem::GetPacket(const MaterialComponent& mc) const
{
	MaterialKey key{
		mc.surface,
		mc.albedo,
		mc.normal,
		mc.metallic,
		mc.roughness
	};

	auto it = cache_.find(key);
	return (it != cache_.end()) ? &it->second : nullptr;
}
