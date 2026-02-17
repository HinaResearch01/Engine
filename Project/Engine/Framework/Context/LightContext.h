#pragma once

#include "Math/TMath.h"
#include <optional>
#include <vector>

namespace Tsumi::Framework {

struct DirectionalLightResolved {
	bool enabled = false;
	Math::Vec3f dirWS;
	Math::Vec3f radiance;
	float intensity; 
	Math::Vec3f ambient;
};

struct PointLightResolved {
	Math::Vec3f positionWS;
	float range;
	Math::Vec3f radiance;
	float intensity;
};

struct SpotLightResolved {
	Math::Vec3f positionWS;
	float range;
	Math::Vec3f directionWS;
	float innerCos;
	float outerCos;
	Math::Vec3f radiance;
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