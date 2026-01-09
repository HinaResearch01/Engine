#pragma once

#include <string>
#include "Math/TMath.h"

namespace Tsumi::Framework {

// 
struct Material {
	std::string albedoTex;
};

// 
struct MaterialInstance {
	const Material* base = nullptr;
	Math::Vec4f baseColor{};
};

}