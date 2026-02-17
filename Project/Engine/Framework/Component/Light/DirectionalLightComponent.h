#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Str/LightType.h"

namespace Tsumi::Framework {

// ShadowParams moved to ShadowComponent

/* ディレクショナルライト情報を管理するコンポーネント */
class DirectionalLightComponent : public IComponent {

public:
	void OnInspectorGui() override;

public:
	LightType type = LightType::Directional;

	Math::Vec3f color{ 1,1,1 };
	float intensity = 1.0f;

	Math::Vec3f ambient{ 0.1f, 0.1f, 0.1f };

};

}
