#pragma once

#include <cstdint>
#include "Framework/Str/RenderSurfaceType.h"
#include "Framework/Str/RenderGpuStructure.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

struct MaterialPacket
{
	// GPUに送る定数
	GpuMaterialCB cb{};

	// GPUリソース
	Tsumi::Resource::TextureAsset* albedo = nullptr;
};

struct RenderPacket
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