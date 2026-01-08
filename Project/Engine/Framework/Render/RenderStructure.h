#pragma once

#include <string>
#include "Math/TMath.h"
#include "Resource/Tex/TextureManager.h"
#include "Resource/Mesh/MeshManager.h"

namespace Tsumi::Framework {

using MeshHandle = std::string;
using TextureHandle = std::string;

// 描画レイヤー
enum class RenderLayer : uint8_t {
	Opaque,
	Transparent,
};

// 
struct GpuTransformCB {
	Math::Mat4x4 world;
};

struct GupCameraCB {
	Math::Mat4x4 view;
	Math::Mat4x4 viewProj;
};

// 
struct RenderItem {
	MeshHandle mesh;
	TextureHandle albedo;
	RenderLayer layer;
	GpuTransformCB transfromCB;
};

// 1 draw に必要な情報をまとめたもの
struct DrawPatcket {
	// 参照
	Tsumi::Resource::MeshAsset* mesh = nullptr;
	Tsumi::Resource::TextureAsset* albedo = nullptr;

	// GpuTransformCB
	GpuTransformCB transformCB{};

	// ソートキー
	uint64_t sortKey = 0;
};

}