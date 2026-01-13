#pragma once

#include "../IComponent.h"

namespace Tsumi::Framework {

/* Actorの「描画」を管理　データコンポネント */
class RenderComponent : public IComponent {

public:
	std::string mesh;
	bool visible = true;
	bool castShadow = true;

	// TODO: bounds / layerMask / submesh 等も後から
};

}