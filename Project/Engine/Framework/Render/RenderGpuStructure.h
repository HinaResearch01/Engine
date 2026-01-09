#pragma once

#include "Math/TMath.h"


namespace Tsumi::Framework {

struct GpuTransformCB {
	Math::Mat4x4 world;
	Math::Mat4x4 worldInvTranspose;
};

struct GpuMaterialCB {
	Math::Vec4f baseColor;
};

struct GupCameraCB {
	Math::Mat4x4 view;
	Math::Mat4x4 proj;
	Math::Mat4x4 viewProj;
};

}