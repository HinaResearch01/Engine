#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Str/LightType.h"

namespace Tsumi::Framework {

/* スポットライト情報を管理するコンポーネント */
class SpotLightComponent : public IComponent {

public:
	void OnInspectorGui() override;

public:
	LightType type = LightType::Spot;

	Math::Vec3f color{ 1,1,1 };
	float intensity = 1.0f;
	float range = 10.0f;

	// 角度
	float innerAngle = 20.0f;
	float outerAngle = 30.0f;

	// GPU 用 (Calculated by LightSystem)
	float innerCos = 0.939f; // cos(20) approx
	float outerCos = 0.866f; // cos(30) approx

	// Shadow (Simple)
	bool castShadow = true;
	float nearZ = 0.5f;
	float farZ = 100.0f;

	// Runtime (Transient)
	mutable int32_t runtimeShadowIndex = -1;
};

}
