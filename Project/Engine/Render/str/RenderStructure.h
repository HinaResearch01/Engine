#pragma once

#include "Math/TMath.h"

namespace Tsumi::Render {

struct GPUTransformCB {
	Math::Mat4x4 worldMat;
	Math::Mat4x4 worldInverseTranspose;
};

}