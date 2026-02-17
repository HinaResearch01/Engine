#pragma once

#include "../../../Math/TMath.h"
#include "../ISystem.h"
#include "Framework/Context/ShadowContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Shadow/ShadowComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Shadow管理クラス */
class ShadowSystem : public ISystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	ShadowSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~ShadowSystem() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::SceneContext; }

#pragma region Accessor
	const ShadowContext& GetContext() const { return activeCtx_; }
#pragma endregion 

private:
	/// <summary>
	/// ShadowContextを組み立てる
	/// 戻り値: 有効なDirectionalLightが見つかったかどうか
	/// </summary>
	bool BuildShadowContext(ShadowContext& out);
	void BuildSpotShadowContext(ShadowContext& out);
	void BuildDefault(ShadowContext& out);

	/// <summary>
	///  CSM の Texel Snapping
	/// Ortho の XY 範囲を texel グリッドにスナップして “泳ぎ” を抑える
	/// </summary>
	void SnapOrthoToTexel(Math::Mat4x4& lightView, float orthoWidth, 
						  float orthoHeight, uint32_t shadowMapSize);

private:
	ShadowContext defaultCtx_{};
	ShadowContext activeCtx_{};
	
	World& world_;
};

}
