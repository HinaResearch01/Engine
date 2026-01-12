#pragma once
#pragma once

#include <unordered_map>
#include <string>
#include "Framework/Render/RenderMaterial.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

class MaterialInstance {
	RenderMaterial* parent = nullptr;
	std::unordered_map<std::string, Resource::TextureAsset*> textures;
};

}