#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "Framework/Update/IUpdatable.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Str/MaterialPacket.h"
#include "Framework/Str/RenderSurfaceType.h"

// 前方宣言
namespace Tsumi::Resource { class ResourceSystem; }

namespace Tsumi::Framework {

// 前方宣言
class World;

struct MaterialKey {
	SurfaceType surface{};
	std::string albedoKey;

	bool operator==(const MaterialKey& r) const
	{
		return surface == r.surface
			&& albedoKey == r.albedoKey;
	}
};

struct MaterialKeyHash
{
	size_t operator()(const MaterialKey& k) const noexcept
	{
		// 最小hash（後でFNV1a等に置換OK）
		size_t h = static_cast<size_t>(k.surface);
		auto mix = [&](size_t v) { h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };

		mix(std::hash<std::string>{}(k.albedoKey));
		return h;
	}
};

class MaterialSystem : public IUpdatable {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	MaterialSystem() = default;
	MaterialSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~MaterialSystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::RenderPrepare; }

	/// <summary>
	/// Packetの取得
	/// </summary>
	const MaterialPacket* GetPacket(const MaterialComponent& mc) const;

private:
	std::unordered_map<MaterialKey, MaterialPacket, MaterialKeyHash> cache_;

	World& world_;
	Resource::ResourceSystem* resourceSys_ = nullptr;
};

}
