#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"

namespace Tsumi::Framework {

/* 影生成情報を管理するコンポーネント */
class ShadowComponent : public IComponent {

public:
	void OnInspectorGui() override;

public:

	// 影を有効にするか
	bool castShadow = true;

	// Shadow Map Size (e.g. 1024, 2048, 4096)
	uint32_t shadowMapSize = 1024;

	// Directional Light Specific (CSM)
	float orthoHalfSize = 50.0f;
	// Near/Far planes for shadow camera
	float nearZ = 0.1f;
	float farZ = 200.0f;

	// ----------------------------------------
	// Spot Light Specific
	// ----------------------------------------
	// ShadowSystemによってフレーム毎に割り当てられるIndex (-1 なら影なし)
	int32_t spotShadowIndex = -1;
	float shadowBias = 0.005f;

	// ----------------------------------------
	// Future: Point/Spot Light specific params (Bias etc.)
	// ----------------------------------------
};

}
