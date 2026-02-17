#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Str/LightType.h"

namespace Tsumi::Framework {

/* スポットライト情報を管理するコンポーネント */
class SpotLightComponent : public IComponent {

public:
	LightType type = LightType::Spot;

	Math::Vec3f color{ 1,1,1 };
	float intensity = 1.0f;
	float range = 10.0f;

	// 角度
	float innerAngle;
	float outerAngle;

	// GPU 用
	float innerCos;
	float outerCos;

	// Inspector
	void OnInspectorGui() override;
};

}
