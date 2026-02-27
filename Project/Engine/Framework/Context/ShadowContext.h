#pragma once

#include "Math/TMath.h"
#include <array>

namespace Tsumi::Framework {

struct ShadowCascade {
	tme::math::Mat4x4 view{};
	tme::math::Mat4x4 proj{};
	tme::math::Mat4x4 viewProj{};
};

struct ShadowContext {
	bool enabled = false;
	uint32_t cascadeCount = 0;
	uint32_t shadowMapSize = 0;
	std::array<float, 4> splitFar{};
	std::array<ShadowCascade, 4> cascades{};

	// Spot Light Shadow
	bool spotEnabled = false;
	uint32_t spotShadowMapSize = 0;
	// 最大数はシェーダー側の配列サイズと合わせる必要あり（仮に16）
	static constexpr uint32_t kMaxSpotShadows = 16;
	struct SpotShadowData {
		tme::math::Mat4x4 view;
		tme::math::Mat4x4 proj;
		tme::math::Mat4x4 viewProj;
		uint32_t lightIndex; // LightSystem側のindexとの対応付け（必要なら）
	};
	uint32_t activeSpotShadowCount = 0;
	std::array<SpotShadowData, kMaxSpotShadows> spotShadows{};
};

/*
CSM : Cascaded Shadow Maps
太陽からの距離(Near Mid Far VeryFar)で四つのマップ
を用意して綺麗な影をつくる技術
*/

}
