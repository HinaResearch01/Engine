#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

struct GpuDirectionalLightCB {
	// 光の向き
	tme::math::Vec3f directionWS;
	int enabled = 0;

	// 放射輝度（HDR）
	tme::math::Vec3f radiance;
	float pad1;

	// 環境光色 (Ambient)
	tme::math::Vec3f ambientColor;
	float pad2;
};

struct GpuPointLightCB {
	// 座標
	tme::math::Vec3f positionWS;
	float range;

	// 放射輝度（HDR）
	tme::math::Vec3f radiance;
	float _pad0;
};

struct GpuSpotLightCB {
	// 座標
	tme::math::Vec3f positionWS;
	float range;

	// 光の向き
	tme::math::Vec3f directionWS;
	float innerCos;

	// 放射輝度（HDR）
	tme::math::Vec3f radiance;
	float outerCos;

	// 拡張
	float intensity;
	int shadowIndex;
	float shadowBias;
	float _pad;

	tme::math::Mat4x4 lightViewProj;
};

struct GpuShadowCSMCB
{
	tme::math::Mat4x4 lightViewProj[4];
	float cascadeSplitDepths[4];
	float shadowTexelSize[2];
	float shadowBias;
	float shadowNormalBias;
};

struct GpuShadowCasterCB
{
	tme::math::Mat4x4 lightViewProj{};
};

struct GpuDebugCB
{
	int mode = 0;
	int channel = 0;
	float value0 = 0.0f;
	float value1 = 0.0f;
};


}
