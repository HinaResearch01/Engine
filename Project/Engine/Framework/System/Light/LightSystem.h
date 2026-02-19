#pragma once

#include "Math/TMath.h"
#include "../ISystem.h"
#include "Framework/Context/LightContext.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* Light管理クラス */
class LightSystem : public ISystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	LightSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~LightSystem() = default;

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
	const LightContext& GetContext() const { return activeCtx_; }
#pragma endregion 

private:
	/// <summary>
	/// LightContextを組み立てる
	/// </summary>
	void BuildLightContext(LightContext& out);
	void BuildDefault(LightContext& out);

private:
	LightContext defaultCtx_;
	LightContext activeCtx_;
};

}
