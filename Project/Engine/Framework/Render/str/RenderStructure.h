#pragma once

#include "Math/TMath.h"

namespace Tsumi::Framework {

struct GPUTransformCB {
	Math::Mat4x4 worldMat;
	Math::Mat4x4 worldInverseTranspose;
};

struct GPUCameraMatricesCB {
	Math::Mat4x4 view;
	Math::Mat4x4 proj;
	Math::Mat4x4 viewProj;
};

struct GPUCameraParamsCB {
	Math::Vec3f cameraPos;
	float nearZ;
	Math::Vec3f cameraDir;
	float farZ;
};

}