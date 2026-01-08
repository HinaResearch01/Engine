#pragma once

#include "../IComponent.h"
#include "Framework/Render/RenderStructure.h"

namespace Tsumi::Framework {

/* Actorの「描画」を管理　データコンポネント */
class RenderComponent : public IComponent {

public:
	RenderItem renderItem{};
};

}