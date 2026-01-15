#pragma once

#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Component/Transform/TransformComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Transform管理クラス */
class TransformSystem : public IUpdatable {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	TransformSystem() = default;;
	TransformSystem(World& world);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::PostLogic; }

private:
	/// <summary>
	/// ノード更新
	/// </summary>
	void UpdateComponent(TransformComponent& tr);

private:
	World& world_;
};

}