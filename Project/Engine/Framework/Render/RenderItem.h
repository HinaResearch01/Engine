#pragma once

#include <cstdint>
#include "Framework/Render/RenderSurfaceType.h"

namespace Tsumi::Framework {

// ソートキー
struct DrawKey {
	uint8_t surface = 0;     // SurfaceType
	uint8_t queue = 0;     // 0: Opaque系 / 1: Transparent系 など
	uint16_t pad = 0;

	uint32_t psoId = 0; // PSOの識別（hashでもindexでも）
	uint32_t materialId = 0;
	uint32_t meshId = 0;

	uint32_t depthKey = 0; // 透明なら遠→近にしたい等で使う
};

struct DrawItem {
	DrawKey key{};

	// 参照
	uint32_t actorId = 0;

	const void* transformPtr = nullptr;

	// 描画に必要な参照
	uint32_t meshHandle = 0;
	uint32_t materialHandle = 0;
};

}