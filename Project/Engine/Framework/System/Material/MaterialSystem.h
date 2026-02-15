#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "../ISystem.h"
#include "Framework/Context/MaterialContext.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Str/RenderPacket.h"
#include "Framework/Str/RenderSurfaceType.h"

// 前方宣言
namespace Tsumi::Resource { class ResourceSystem; }

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Material管理クラス */
class MaterialSystem : public ISystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
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
	UpdatePhase Phase() const override { return UpdatePhase::SceneContext; }

#pragma region Accessor
	const MaterialContext& GetContext() const { return ctx_; }
#pragma endregion

private:
	MaterialContext ctx_;

	Resource::ResourceSystem* resourceSys_ = nullptr;
	World& world_;
};

}
