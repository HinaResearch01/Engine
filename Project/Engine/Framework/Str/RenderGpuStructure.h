#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

struct GpuTransformCB
{
	Math::Mat4x4 world;
	Math::Mat4x4 worldInvTranspose;
};

struct GpuMaterialUVCB
{
	Math::Mat3x3 uvTransform;
};

struct GpuMaterialParamsCB
{
	Math::Vec3f baseColor;
	float alpha;

	float roughness;
	float metallic;
	float ao;
	float useAlbedoTex;
};

struct GpuCameraCB
{
	Math::Mat4x4 view;
	Math::Mat4x4 proj;
	Math::Mat4x4 viewProj;
	Math::Mat4x4 invView;
	Math::Mat4x4 invProj;
	Math::Mat4x4 invViewProj;
	Math::Vec3f cameraPos;
	float pad;
};

}