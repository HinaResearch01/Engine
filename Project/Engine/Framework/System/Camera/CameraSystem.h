#pragma once

#include "Math/TMath.h"
#include "../ISystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/World/CompView/ComponentView.h"

// 前方宣言
namespace Tsumi::DX12 { class DX12Manager; }

namespace Tsumi::Framework {

class World;
class IActor;
class CameraComponent;

/* カメラシステム */
class CameraSystem : public ISystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CameraSystem() = delete;
	CameraSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~CameraSystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override {
		return UpdatePhase::SceneContext;
	}

#pragma region Accessor
	const CameraContext& GetContext() const { return activeCtx_; }
#pragma endregion

private:
	/// <summary>
	/// 使用するカメラの選択
	/// </summary>
	IActor* SelectCamera() const;

	/// <summary>
	/// CameraActorからのContext構築
	/// </summary>
	void BuildFromActor(IActor* actor, CameraContext& out);

	/// <summary>
	/// デフォルトカメラの構築
	/// </summary>
	void BuildDefault(CameraContext& out);

private:
	Math::Mat4x4 MakeViewProj(const Math::Mat4x4& view, const Math::Mat4x4& proj);

private:
	CameraContext activeCtx_{};
	CameraContext defaultCtx_{};

	World& world_;
};

}