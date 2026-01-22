#pragma once

#include "Math/TMath.h"
#include <optional>
#include <vector>
#include "Framework/Str/LightPacket.h"

namespace Tsumi::Framework {

struct DirectionalLightResolved {
	bool enabled = false;
	Math::Vec3f dirWS;
	Math::Vec3f radiance;
};

struct PointLightResolved {
	Math::Vec3f positionWS;
	float range;
	Math::Vec3f radiance;
};

struct SpotLightResolved {
	Math::Vec3f positionWS;
	float range;
	Math::Vec3f directionWS;
	float innerCos;
	float outerCos;
	Math::Vec3f radiance;
};

struct LightContext {
	DirectionalLightResolved directional;
	std::vector<PointLightResolved> points;
	std::vector<SpotLightResolved> spots;

	LightPacket packet;
};

}