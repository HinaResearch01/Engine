#pragma once

#include "Math/TMath.h"
#include "../ISystem.h"
#include "Framework/Context/ShadowContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Shadow管理クラス */
class ShadowSystem : public ISystem {

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
	UpdatePhase Phase() const override { return UpdatePhase::SceneContext; }

#pragma region Accessor
	const ShadowContext& GetContext() const { return context_; }
#pragma endregion 

private:
	/// <summary>
	/// ShadowContextを組み立てる
	/// </summary>
	void BuildShadowContext();

	/// <summary>
	///  CSM の Texel Snapping
	/// Ortho の XY 範囲を texel グリッドにスナップして “泳ぎ” を抑える
	/// </summary>
	void SnapOrthoToTexel(Math::Mat4x4& lightView, float orthoWidth, 
						  float orthoHeight, uint32_t shadowMapSize);

private:
	ShadowContext context_{};
	
	World& world_;
};

}
