#pragma once

#include "Math/TMath.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

struct RenderItem {
	Resource::MeshAsset* mesh = nullptr;
	Resource::TextureAsset* texture = nullptr;
	float depth = 0.0f;
};

}