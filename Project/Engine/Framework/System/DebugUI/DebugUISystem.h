#pragma once

#include "../ISystem.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Debug用のImGui表示管理 */
class DebugUISystem : public ISystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	DebugUISystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~DebugUISystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phase
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::DebugUI; }

private:
	World& world_;
};
}