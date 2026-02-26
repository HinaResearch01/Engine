#pragma once

#include <unordered_map>
#include <string>
#include "Math/TMath.h"
#include "Framework/Str/RenderGpuStructure.h"
#include "Framework/Str/RenderSurfaceType.h"

// 前方宣言
namespace tme::sys::resource { struct TextureAsset;  }

namespace Tsumi::Framework {

struct MaterialKey {
	SurfaceType surface{};
	std::string albedoKey;

	bool operator==(const MaterialKey& r) const {
		return surface == r.surface && albedoKey == r.albedoKey;
	}
};

struct MaterialKeyHash {
	size_t operator()(const MaterialKey& k) const noexcept {
		size_t h = static_cast<size_t>(k.surface);
		auto mix = [&](size_t v) { h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); };
		mix(std::hash<std::string>{}(k.albedoKey));
		return h;
	}
};

struct MaterialResolved {
	GpuMaterialUVCB      uv;
	GpuMaterialParamsCB  params;
	tme::sys::resource::TextureAsset* albedo = nullptr;
};

struct MaterialContext {
	std::unordered_map<MaterialKey, MaterialResolved, MaterialKeyHash> cache;

	void Clear() { cache.clear(); }

	const MaterialResolved* Find(const MaterialKey& key) const {
		auto it = cache.find(key);
		return (it != cache.end()) ? &it->second : nullptr;
	}
};


}