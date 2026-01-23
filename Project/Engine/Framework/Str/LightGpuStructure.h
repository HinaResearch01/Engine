#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

struct GpuDirectionalLightCB {
	// 光の向き
	Math::Vec3f directionWS;
	int enabled = 0;

	// 放射輝度（HDR）
	Math::Vec3f radiance;
	float pad1;

	// shadow 有無
	uint32_t castShadow;
	Math::Vec3f pad2;
};

struct GpuPointLightCB {
	// 座標
	Math::Vec3f positionWS;
	float range;

	// 放射輝度（HDR）
	Math::Vec3f radiance;
	float _pad0;
};

struct GpuSpotLightCB {
	// 座標
	Math::Vec3f positionWS;
	float range;

	// 光の向き
	Math::Vec3f directionWS;
	float innerCos;

	// 放射輝度（HDR）
	Math::Vec3f radiance;
	float outerCos;
};

struct GpuShadowCSMCB
{
	int enabled;
	int cascadeCount;
	float shadowMapSize;
	float invShadowMapSize;

	float splitFar[4];

	// 各カスケードの行列
	Math::Mat4x4 shadowViewProj[4];
};

struct GpuShadowCasterCB
{
	Math::Mat4x4 lightViewProj{};
};

struct GpuDebugCB
{
	int mode = 0;
	int channel = 0;
	float value0 = 0.0f;
	float value1 = 0.0f;
};


}
