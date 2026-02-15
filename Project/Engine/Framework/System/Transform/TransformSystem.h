#pragma once

#include <unordered_set>
#include "Math/TMath.h"
#include "../ISystem.h"
#include "Framework/Component/Transform/TransformComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Transform管理クラス */
class TransformSystem : public ISystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	TransformSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~TransformSystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::Transform; }

private:
	/// <summary>
	/// 
	/// </summary>
	void UpdateHierarchy(TransformComponent& tr);

	/// <summary>
	/// ノード更新
	/// </summary>
	void UpdateComponent(TransformComponent& tr);

private:
	World& world_;
};

}