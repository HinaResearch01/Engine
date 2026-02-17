#pragma once

#include "../IComponent.h"
#include "Math/TMath.h"
#include "Framework/Str/RenderSurfaceType.h"

namespace Tsumi::Framework {

using MaterialHandle = uint32_t;

/* マテリアル情報を管理するコンポーネント */
class MaterialComponent : public IComponent {

public:
	void OnInspectorGui() override;

public:
	SurfaceType surface = SurfaceType::Opaque;

	// PBR 
	Math::Vec3f baseColor = { 1.0f, 1.0f, 1.0f };
	float alpha = 1.0f;
	float roughness = 1.0f;
	float metallic = 0.0f;
	float ao = 1.0f;

	float useAlbedoTex = 1.0f;

	// Texture 
	std::string albedo;

	// UV 
	Math::UVTransform uv;

	bool visible = true;

};

}