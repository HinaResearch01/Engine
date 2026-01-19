#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

struct GpuTransformCB
{
	Math::Mat4x4 world;
	Math::Mat4x4 worldInvTranspose;
};

struct GpuMaterialCB
{
	Math::Vec4f color;
	Math::Mat3x3 uvTransform;
};

struct GpuViewCB
{
	Math::Mat4x4 view;
	Math::Mat4x4 proj;
	Math::Mat4x4 viewProj;
	Math::Vec3f cameraPos;
	float pad;
};

}