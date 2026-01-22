#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

struct GpuDirectionalLightCB {
	// 光の向き（正規化済み）
	Math::Vec3f directionWS;
	float pad0;

	// 放射輝度（HDR）
	Math::Vec3f radiance;
	float pad1;

	// shadow 有無
	uint32_t castShadow;
	Math::Vec3f pad2;
};

struct GpuPointLightCB {}; // TODO

struct GpuSpotLightCB {}; // TODO

}
