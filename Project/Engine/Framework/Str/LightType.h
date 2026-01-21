#pragma once
#include <cstdint>

namespace Tsumi::Framework {

enum class LightType : uint8_t {
	Directional,
	Point,
	Spot
};

}