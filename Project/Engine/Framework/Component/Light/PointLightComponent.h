#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Str/LightType.h"

namespace Tsumi::Framework {

/* ポイントライト情報を管理するコンポーネント */
class PointLightComponent : public IComponent {

public:
	LightType type = LightType::Point;

	Math::Vec3f color{ 1,1,1 };
	float intensity = 1.0f;
	float range = 10.0f;
};

}
