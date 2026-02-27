#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

// カメラ行列
struct GpuCameraMatricesCB {
	tme::math::Mat4x4 view{};
	tme::math::Mat4x4 proj{};
	tme::math::Mat4x4 viewProj{};
	tme::math::Mat4x4 invView{};
	tme::math::Mat4x4 invProj{};
	tme::math::Mat4x4 invViewProj{};
};

// カメラパラメータ
struct GpuCameraParameterCB {
	tme::math::Vec3f position{};
	tme::math::Vec3f forward{};
	float fovY = 0.0f;
	float aspectRatio = 0.0f;
	float nearPlane = 0.0f;
	float farPlane = 0.0f;
};

}
