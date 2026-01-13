#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Render/RenderSurfaceType.h"

namespace Tsumi::Framework {

using MaterialHandle = uint32_t;

/* マテリアル情報を管理するコンポーネント */
class MaterialComponent : public IComponent {

public:
	SurfaceType surface = SurfaceType::Opaque;

	Math::Vec4f baseColor{};

	std::string albedo = "";
	std::string normal = "";

	float metallic = 0.0f;
	float roughness = 1.0f;

	bool visible = true;
};

}