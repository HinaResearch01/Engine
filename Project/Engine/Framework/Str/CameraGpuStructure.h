#pragma once

#include <cstdint>
#include "Math/TMath.h"

namespace Tsumi::Framework {

// カメラ行列
struct GpuCameraMatricesCB {
	Math::Mat4x4 view{};
	Math::Mat4x4 proj{};
	Math::Mat4x4 viewProj{};
	Math::Mat4x4 invView{};
	Math::Mat4x4 invProj{};
	Math::Mat4x4 invViewProj{};
};

// カメラパラメータ
struct GpuCameraParameterCB {
	Math::Vec3f position{};
	Math::Vec3f forward{};
	float fovY = 0.0f;
	float aspectRatio = 0.0f;
	float nearPlane = 0.0f;
	float farPlane = 0.0f;
};

}
