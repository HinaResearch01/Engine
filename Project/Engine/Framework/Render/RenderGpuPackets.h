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
	Math::Vec4f baseColor;
	float metallic = 0.0f;
	float roughness = 1.0f;
	uint32_t flags = 0;
	uint32_t pad = 0;
};

}