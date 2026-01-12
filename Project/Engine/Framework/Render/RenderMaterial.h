#pragma once

#include <unordered_map>
#include <string>
#include "Math/TMath.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

enum class BlendMode {
	Opaque,
	Masked,
	Translucent
};

enum class ShadingModel {
	DefaultLit,
	Unlit
};

struct RenderMaterial {
	std::string shaderName;          // "Standard"
	BlendMode   blendMode = BlendMode::Opaque;
	ShadingModel shadingModel = ShadingModel::DefaultLit;
	bool twoSided = false;
};

}