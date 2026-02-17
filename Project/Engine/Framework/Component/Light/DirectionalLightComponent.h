#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Str/LightType.h"

namespace Tsumi::Framework {

struct ShadowParams {
	bool castShadow = true;
	uint32_t shadowMapSize = 2048;
	// Directional shadow camera params
	float orthoHalfSize = 50.0f;   // 影を写す範囲（半径）
	float nearZ = 0.1f;
	float farZ = 200.0f;
};

/* ディレクショナルライト情報を管理するコンポーネント */
class DirectionalLightComponent : public IComponent {

public:
	LightType type = LightType::Directional;

	Math::Vec3f color{ 1,1,1 };
	float intensity = 1.0f;

	Math::Vec3f ambient{ 0.1f, 0.1f, 0.1f };

	ShadowParams shadow;
};

}
