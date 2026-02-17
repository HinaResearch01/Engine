#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Str/LightType.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Component/Transform/TransformComponent.h"

namespace Tsumi::Framework {

/* ディレクショナルライト情報を管理するコンポーネント */
class DirectionalLightComponent : public IComponent {

public:
	DirectionalLightComponent() = default;

	void OnInspectorGui() override;

	void Init() override {
		UpdateTransform();
	}

	void UpdateTransform() {
		if (auto* owner = GetOwner()) {
			if (auto* tr = owner->GetComponent<TransformComponent>()) {
				tr->srt.rotate.x = elevation;
				tr->srt.rotate.y = azimuth;
				tr->MarkDirty();    
			}
		}
	}

public:
	Math::Vec3f color{ 1,1,1 };
	float intensity = 2.5f;

	Math::Vec3f ambient{ 0.1f, 0.1f, 0.1f };

	// Sun Control Params
	// Elevation (Pitch): -90 to 90
	float elevation = 45.0f; 
	// Azimuth (Yaw): 0 to 360
	float azimuth = 315.0f;

	// Shadow (Simple)
	bool castShadow = true;
	float orthoHalfSize = 50.0f;
	float nearZ = 0.1f;
	float farZ = 200.0f;
};

}
