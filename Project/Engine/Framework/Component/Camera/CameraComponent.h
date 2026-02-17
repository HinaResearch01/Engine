#pragma once

#include "Math/TMath.h"
#include "../IComponent.h"

namespace Tsumi::Framework {

/* カメラを管理 */
class CameraComponent : public IComponent {

public:
	void OnInspectorGui() override;

public:
	float fovY = 60.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
	
	bool active = true;
	bool mainCandidate = true;
	int  priority = 0;
};

}