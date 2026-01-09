#pragma once

#include "../IComponent.h"
#include "Framework/Render/RenderMaterial.h"
#include "Framework/Render/RenderSortKey.h"

namespace Tsumi::Framework {

/* Actorの「描画」を管理　データコンポネント */
class RenderComponent : public IComponent {

public:
	std::string mesh;              // Mesh key
	MaterialInstance material;     // 個体差を含む

	RenderQueue renderQueue = RenderQueue::Opaque;
	uint32_t layerMask = RenderLayer::Default;
	bool visible = true;
};

}