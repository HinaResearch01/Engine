#pragma once

#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Context/ShadowContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Shadow管理クラス */
class ShadowSystem : public IUpdatable {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	ShadowSystem() = default;;
	ShadowSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~ShadowSystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::ShadowSys; }

#pragma region Accessor
	const ShadowContext& GetContext() const { return context_; }
#pragma endregion 

private:
	/// <summary>
	/// ShadowContextを組み立てる
	/// </summary>
	void BuildShadowContext();

private:
	ShadowContext context_{};
	
	World& world_;
};

}
