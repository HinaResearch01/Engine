#pragma once

#include <cstdint>

namespace Tsumi::Framework {

enum class SurfaceType : uint8_t
{
	Opaque = 0,
	Cutout,
	Transparent,
	ShadowCaster,
	Skybox,
	UI,

	Count
};

}
