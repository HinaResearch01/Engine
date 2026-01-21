#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"

namespace Tsumi::Framework {

struct ShadowSettings
{
	uint32_t resolution = 2048;
	float nearZ = 0.1f;
	float farZ = 200.0f;

	// 方向光の正射影サイズ（ワールド単位）
	float orthoSize = 50.0f;

	// ライト“カメラ”をどれだけ後ろに引くか
	float cameraDistance = 100.0f;
};

/* ディレクショナルライト情報を管理するコンポーネント */
class DirectionalLightComponent : public IComponent {

public:
	Math::Vec3f direction = { 0.0f, -1.0f, 0.0f }; // World space（正規化前提）
	Math::Vec3f color = { 1.0f,  1.0f, 1.0f };
	float intensity = 1.0f;

	bool castShadow = true;

	ShadowSettings shadow{};
};

}
