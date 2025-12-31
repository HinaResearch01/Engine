#pragma once

#include "Math/TMath.h"

namespace Tsumi::Render {

struct GPUTransformCB {
	Math::Mat4x4 worldMat;
	Math::Mat4x4 worldInverseTranspose;
};

struct CameraMatricesCB {
	Math::Mat4x4 view;
	Math::Mat4x4 proj;
	Math::Mat4x4 viewProj;
};

struct CameraParamsCB {
	Math::Vec3f cameraPos;
	float nearZ;
	Math::Vec3f cameraDir;
	float farZ;
};

}