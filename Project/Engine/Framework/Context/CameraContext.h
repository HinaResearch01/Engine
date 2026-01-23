#pragma once

#include "Math/TMath.h"

namespace Tsumi::Framework {

// カメラ情報
struct CameraContext {
	bool valid = false;

	Math::Mat4x4 view{};
	Math::Mat4x4 proj{};
	Math::Mat4x4 viewProj{};

	Math::Vec3f position{};
	Math::Vec3f forward{};

	float fovY = 60.0f;
	float aspectRatio = 16.0f / 9.0f;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
};

}