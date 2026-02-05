#pragma once

#include <cstdint>
#include "Framework/Str/RenderSurfaceType.h"
#include "Framework/Str/RenderGpuStructure.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

struct RenderPacket
{
	SurfaceType surface{};

	// Geometry
	Tsumi::Resource::MeshAsset* mesh = nullptr;

	// Material
	GpuMaterialUVCB materialUVCB{};
	GpuMaterialParamsCB materialParamsCB{};
	Tsumi::Resource::TextureAsset* albedo = nullptr;

	// Transform
	GpuTransformCB xform{};

	bool castShadow = true;

	// Sort
	uint64_t sortKey = 0;
};

}