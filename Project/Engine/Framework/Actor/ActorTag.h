#pragma once
#include <cstdint>

namespace Tsumi::Framework {

enum class ActorTag : uint8_t {
	None = 0,
	Player,
	Enemy,
	Bullet,
	Terrain,
};

}