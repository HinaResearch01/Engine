#pragma once

#include "Framework/Str/RenderGpuPackets.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

struct MaterialPacket
{
	// GPUに送る定数
	GpuMaterialCB cb{};

	// GPUリソース
	Tsumi::Resource::TextureAsset* albedo = nullptr;
};

}