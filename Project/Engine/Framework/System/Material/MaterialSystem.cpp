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
		};

		auto& pkt = cache_[key];

		// GPU material CB
		pkt.cb.color = mc.color;
		pkt.cb.uvTransform = Math::Func::MAT3x3::BuildUVMatrix(mc.uv);

		// TextureManager から GPU リソースを取得
		pkt.albedo = mc.albedo.empty()
			? nullptr
			: resourceSys_->GetTextureManager()->GetTexture(mc.albedo);
	}
}

const MaterialPacket* MaterialSystem::GetPacket(const MaterialComponent& mc) const
{
	MaterialKey key{
		mc.surface,
		mc.albedo,
	};

	auto it = cache_.find(key);
	return (it != cache_.end()) ? &it->second : nullptr;
}
