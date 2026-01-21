#pragma once

#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Context/ShadowContext.h"
#include "Framework/Component/Light/LightComponent.h"

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
	UpdatePhase Phase() const override { return UpdatePhase::RenderPrepare; }

#pragma region Accessor
	const ShadowContext& GetShadowContext() const { return context_; }
#pragma endregion 

private:
	/// <summary>
	/// 
	/// </summary>
	Math::Mat4x4 BuildOrthoDX(float halfSize, float nearZ, float farZ) const;

	/// <summary>
	///  影ビュー行列
	/// </summary>
	Math::Mat4x4 BuildLightView(const Math::Vec3f& dirWS, const Math::Vec3f& centerWS) const;

private:
	World& world_;

	ShadowContext context_{};
};

}
