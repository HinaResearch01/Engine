#pragma once

#include <cstdint>

namespace Tsumi::Framework {

enum class SurfaceType : uint8_t {
	Opaque,
	Masked,
	Translucent,
	Additive
};

}