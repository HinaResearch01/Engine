#pragma once

#include "Math/TMath.h"
#include <array>

namespace Tsumi::Framework {

struct ShadowCascade {
	Math::Mat4x4 view{};
	Math::Mat4x4 proj{};
	Math::Mat4x4 viewProj{};
};

struct ShadowContext {
	bool enabled = false;
	uint32_t cascadeCount = 0;
	uint32_t shadowMapSize = 0;
	std::array<ShadowCascade, 4> cascades{};
};

/*
CSM : Cascaded Shadow Maps
太陽からの距離(Near Mid Far VeryFar)で四つのマップ
を用意して綺麗な影をつくる技術
*/

}
