#pragma once

#include <cstdint>

namespace Tsumi::Framework {

enum class RenderQueue : uint16_t {
	Opaque = 2000,
	AlphaTest = 2450,
	Transparent = 3000,
};

enum RenderLayer : uint32_t {
	Default = 1 << 0,
	UI = 1 << 1,
	Shadow = 1 << 2,
};

//uint64_t MakeSortKey(RenderQueue q, const Material* m, const MeshAsset* mesh)
//{
//	uint64_t key = 0;
//	key |= (uint64_t(q) & 0xFFFF) << 48;
//	key |= (uint64_t(m) & 0xFFFFFFFF) << 16;
//	key |= (uint64_t(mesh) & 0xFFFF);
//	return key;
//}

}