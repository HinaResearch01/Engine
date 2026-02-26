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
	tme::sys::resource::MeshAsset* mesh = nullptr;

	// Material
	GpuMaterialUVCB materialUVCB{};
	GpuMaterialParamsCB materialParamsCB{};
	tme::sys::resource::TextureAsset* albedo = nullptr;

	// Transform
	GpuTransformCB xform{};

	bool castShadow = true;

	// Sort
	uint64_t sortKey = 0;
};

}