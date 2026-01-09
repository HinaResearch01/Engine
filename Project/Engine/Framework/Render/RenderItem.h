#pragma once

#include <cstdint>

#include "Math/TMath.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

struct RenderItem {
	uint64_t sortKey;

	Tsumi::Resource::MeshAsset* mesh = nullptr;
	Tsumi::Resource::TextureAsset* albedo = nullptr;

	Math::Mat4x4 world{};
	Math::Vec4f baseColor{};
};

}