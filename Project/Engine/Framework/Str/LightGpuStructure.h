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

	// 環境光色 (Ambient)
	Math::Vec3f ambientColor;
	float pad2;
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
	Math::Mat4x4 lightViewProj[4];
	float cascadeSplitDepths[4];
	float shadowTexelSize[2];
	float shadowBias;
	float shadowNormalBias;
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
