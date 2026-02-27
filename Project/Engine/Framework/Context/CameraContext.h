#pragma once

#include "Math/TMath.h"

namespace Tsumi::Framework {

// カメラ情報
struct CameraContext {
	bool valid = false;

	tme::math::Mat4x4 view{};
	tme::math::Mat4x4 proj{};
	tme::math::Mat4x4 viewProj{};
	tme::math::Mat4x4 invView{};
	tme::math::Mat4x4 invProj{};
	tme::math::Mat4x4 invViewProj{};

	tme::math::Vec3f position{};
	tme::math::Vec3f forward{};
	float fovY = 60.0f;
	float aspectRatio = 16.0f / 9.0f;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
};

}