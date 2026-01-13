#pragma once
#include <array>
#include <string_view>
#include "Framework/Render/RenderSurfaceType.h"

namespace Tsumi::Framework {

struct RenderPassDesc
{
	std::string_view psoName;   // PSOLibraryのキー
	std::string_view rootName;  // RootSignatureLibraryのキー
	bool transparent = false;
};

class RenderPassTable
{
public:
	static const RenderPassDesc& Get(SurfaceType s)
	{
		return table_[static_cast<size_t>(s)];
	}

private:
	static inline const std::array<RenderPassDesc, static_cast<size_t>(SurfaceType::Count)> table_ = {
		RenderPassDesc{ "Object3D_Opaque",       "Object3D", false }, // Opaque
		RenderPassDesc{ "Object3D_Cutout",       "Object3D", false }, // Cutout
		RenderPassDesc{ "Object3D_Transparent",  "Object3D", true  }, // Transparent
		RenderPassDesc{ "ShadowCaster",         "Shadow",   false }, // ShadowCaster
		RenderPassDesc{ "Skybox",               "Skybox",   false }, // Skybox
		RenderPassDesc{ "UI",                   "UI",       true  }, // UI
	};
};

}