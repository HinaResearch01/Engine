#pragma once

#include "Math/TMath.h"

namespace Tsumi::Framework {

struct DirectionalLightData {
	Math::Vec3f direction;
	Math::Vec3f color;
	float intensity;
	bool castShadow;
};

// ライト情報
struct LightContext {
	bool hasDirectional = false;
	DirectionalLightData mainDirectional;
	// 将来: point / spot 配列
};

}