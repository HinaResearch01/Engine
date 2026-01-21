#pragma once

#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Context/LightContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Light管理クラス */
class LightSystem : public IUpdatable {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	LightSystem() = default;;
	LightSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~LightSystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::RenderPrepare; }

#pragma region Accessor
	const LightContext& GetLightContext() const { return context_; }
#pragma endregion 

private:
	World& world_;

	LightContext context_{};
};

}
