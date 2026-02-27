#pragma once

#include "../Vector/Vector.h"

namespace tme::math::constant {
// 円周率関連
constexpr float PI = 3.14159265358979323846f;
constexpr float Double_PI = PI * 2.0f;
constexpr float Half_PI = PI * 0.5f;

namespace Direction {
constexpr Vec3f Front{ 0.0f, 0.0f, 1.0f };
constexpr Vec3f Back{ 0.0f, 0.0f, -1.0f };
constexpr Vec3f Left{ -1.0f, 0.0f, 0.0f };
constexpr Vec3f Right{ 1.0f, 0.0f, 0.0f };
constexpr Vec3f FrontLeft{ -1.0f, 0.0f, 1.0f };
constexpr Vec3f FrontRight{ 1.0f, 0.0f, 1.0f };
constexpr Vec3f BackLeft{ -1.0f, 0.0f, -1.0f };
constexpr Vec3f BackRight{ 1.0f, 0.0f, -1.0f };
}
}