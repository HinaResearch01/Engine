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

	Math::Vec4f color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::UVTransform uv;

	std::string albedo = "";

	bool visible = true;
};

}