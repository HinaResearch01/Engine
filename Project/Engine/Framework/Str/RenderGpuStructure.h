#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

struct GpuTransformCB
{
	tme::math::Mat4x4 world;
	tme::math::Mat4x4 worldInvTranspose;
};

struct GpuMaterialUVCB
{
	tme::math::Mat3x3 uvTransform;
};

struct GpuMaterialParamsCB
{
	tme::math::Vec3f baseColor;
	float alpha;

	float roughness;
	float metallic;
	float ao;
	float useAlbedoTex;
};

struct GpuCameraCB
{
	tme::math::Mat4x4 view;
	tme::math::Mat4x4 proj;
	tme::math::Mat4x4 viewProj;
	tme::math::Mat4x4 invView;
	tme::math::Mat4x4 invProj;
	tme::math::Mat4x4 invViewProj;
	tme::math::Vec3f cameraPos;
	float pad;
};

}