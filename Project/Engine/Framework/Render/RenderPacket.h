#pragma once

#include <cstdint>
#include "Framework/Render/RenderSurfaceType.h"
#include "Framework/Render/RenderGpuPackets.h"
#include "Framework/Render/MaterialPacket.h"
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