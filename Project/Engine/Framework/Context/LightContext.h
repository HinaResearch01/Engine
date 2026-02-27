#pragma once

#include "Math/TMath.h"
#include <optional>
#include <vector>

namespace Tsumi::Framework {

struct DirectionalLightResolved {
	bool enabled = false;
	tme::math::Vec3f dirWS;
	tme::math::Vec3f radiance;
	tme::math::Vec3f ambient;
};

struct PointLightResolved {
	tme::math::Vec3f positionWS;
	float range;
	tme::math::Vec3f radiance;
};

struct SpotLightResolved {
	tme::math::Vec3f positionWS;
	float range;
	tme::math::Vec3f directionWS;
	float innerCos;
	float outerCos;
	tme::math::Vec3f radiance;
	float intensity;
	// Shadow
	int shadowIndex = -1;
	float shadowBias = 0.001f;
};

struct LightContext {
	DirectionalLightResolved directional;
	std::vector<PointLightResolved> points;
	std::vector<SpotLightResolved> spots;
};

}