#pragma once

#include "../IComponent.h"

namespace Tsumi::Resource { 
struct MeshAsset;
}

namespace Tsumi::Framework {

/* Actorの「描画」を管理　データコンポネント */
class RenderComponent : public IComponent {

public:
	std::string mesh = "";
	//AABB bounds{}; // TODO

	uint32_t layerMask = 0xFFFFFFFF;
	bool castShadow = true;
	bool visible = true;
};

}