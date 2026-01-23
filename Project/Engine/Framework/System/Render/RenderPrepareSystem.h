#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/Str/LightPacket.h"
#include "Framework/Str/RenderPacket.h"
#include "Framework/Str/RenderSurfaceType.h"

// 前方宣言
namespace Tsumi::Resource {
class ResourceSystem;
struct MeshAsset;
}

namespace Tsumi::Framework {

class World;
class CameraSystem;
class MaterialSystem;

/* RenderContexを構築するクラス */
class RenderPrepareSystem : public IUpdatable {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	RenderPrepareSystem() = default;
	RenderPrepareSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RenderPrepareSystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::RenderPrepareSys; }

#pragma region Accessor
	const auto& GetRenderPackets() const { return renderPackets_; }
	const LightPacket& GetLightPacket() const { return lightPacket_; }
	const ShadowPacket& GetShadowPacket() const { return shadowPacket_; }
#pragma endregion

private:
	/// <summary>
	/// 
	/// </summary>
	void Clear();

	/// <summary>
	/// Packetの構築
	/// </summary>
	void BuildRenderPackets();

	/// <summary>
	/// 
	/// </summary>
	void BuildLightPacket();

	/// <summary>
	/// 
	/// </summary>
	void BuildShadowPacket();

	/// <summary>
	/// RenderPacketsのソート
	/// </summary>
	void SortRenderPackets();

	/// <summary>
	/// 
	/// </summary>
	void FillTransformPacket(RenderPacket& pkt, const TransformComponent& tc);

private:
	std::array<std::vector<RenderPacket>, static_cast<size_t>(SurfaceType::Count)> renderPackets_;
	LightPacket lightPacket_{};
	ShadowPacket shadowPacket_{};

	World& world_;
	Resource::ResourceSystem* resourceSys_ = nullptr;
};

}