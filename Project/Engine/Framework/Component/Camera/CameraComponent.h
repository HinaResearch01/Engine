#pragma once

#include "Math/TMath.h"
#include "../IComponent.h"

namespace Tsumi::Framework {

struct CameraParams {
	float fovY;
	float nearZ;
	float farZ;
	CameraParams() :
		fovY{ 60.0f },
		nearZ{ 0.1f },
		farZ{ 1000.0f }
	{}
};

struct CameraMatrices {
	Math::Mat4x4 viewMat;
	Math::Mat4x4 projMat;
	Math::Mat4x4 viewProjMat;
	CameraMatrices() :
		viewMat{ Math::Mat4x4::Identity() },
		projMat{ Math::Mat4x4::Identity() },
		viewProjMat{ Math::Mat4x4::Identity() }
	{}
};

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
	void Init() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Update() override;

#pragma region Accessor
	
#pragma endregion 

private:
	// カメラパラメータ
	CameraParams params_;
	// カメラ行列
	CameraMatrices matrices_;

	float priority = 0.0f;
	bool  enabled = true;
};

}