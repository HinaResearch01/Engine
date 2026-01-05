#pragma once

#include "Math/TMath.h"
#include "Framework/Scene/CompView/ComponentView.h"

namespace Tsumi::Framework {

// 前方宣言
class IScene;
class IActor;
class CameraComponent;

// カメラ情報
struct CameraContext {
	bool valid = false;
	Math::Mat4x4 view{};
	Math::Mat4x4 proj{};
	Math::Mat4x4 viewProj{};
	Math::Vec3f position;
};

/* カメラシステム */
class CameraSystem {

private: // シングルトン
	CameraSystem() = default;
	~CameraSystem() = default;
	CameraSystem(const CameraSystem&) = delete;
	const CameraSystem operator=(const CameraSystem&) = delete;

public:
	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(IScene& scene);

private:
	/// <summary>
	/// Mainカメラの選択
	/// </summary>
	IActor* SelectMainCamera(const ComponentView<CameraComponent>& cameras);

	/// <summary>
	/// Matrix群の構築
	/// </summary>
	void BuildMatrices(IActor* actor, CameraContext& out);

private:
	CameraComponent* activeCamera_ = nullptr;
};

}