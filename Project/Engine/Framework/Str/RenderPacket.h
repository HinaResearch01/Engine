#pragma once

#include <cstdint>
#include "Framework/Str/RenderSurfaceType.h"
#include "Framework/Str/RenderGpuPackets.h"
#include "Framework/Str/MaterialPacket.h"
#include "Resource/Mesh/MeshManager.h"

namespace Tsumi::Framework {

struct DrawPacket
{
	SurfaceType surface{};

	// GPU参照
	Tsumi::Resource::MeshAsset* mesh = nullptr;
	const MaterialPacket* material = nullptr;

	GpuTransformCB xform{};

	// ソート用キー
	uint64_t sortKey = 0;
};

}