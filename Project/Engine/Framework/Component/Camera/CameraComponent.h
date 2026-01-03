#pragma once

#include "Math/TMath.h"
#include "../IComponent.h"

namespace Tsumi::Framework {

/* カメラを管理 */
class CameraComponent : public IComponent {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CameraComponent() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~CameraComponent() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init() override {};

public:
	float fovY = 60.0f;
	float aspectRatio = 16.0f / 9.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
	
	bool  enabled = true;

	enum class Role {
		Main,
		Debug,
		CutScene,
		Shadow,
		UI,
	} role = Role::Main;

	int priority = 0;
};

}