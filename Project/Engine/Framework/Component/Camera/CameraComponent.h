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
	
	bool  active = true;
	// Mainが複数ある場合の優先度（大きいほど優先）
	int priority = 0;
	// ゲームカメラとして使用するならtrue
	bool mainCandidate = true;

	enum class Role {
		Invalid = 0,
		Main,
		Sub,
		Shadow,
	} role = Role::Invalid;
};

}