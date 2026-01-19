#pragma once

#include "../Vector/Vector.h"
#include <cmath>
#include <algorithm>

namespace Tsumi::Math {

struct UVTransform {
	Vec2f scale = { 1.0f, 1.0f };
	float rotation = 0.0f;
	Vec2f translate = { 0.0f, 0.0f };
};

}