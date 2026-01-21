#pragma once

#include "Math/TMath.h"

namespace Tsumi::Framework {

// シャドウ情報
struct ShadowContext {
	bool valid = false;

	Math::Mat4x4 view{};
	Math::Mat4x4 proj{};
	Math::Mat4x4 viewProj{};

	Math::Vec3f position{};
	Math::Vec3f forward{};
};

}
