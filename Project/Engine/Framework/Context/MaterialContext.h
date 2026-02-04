#pragma once

#include <unordered_map>
#include <string>
#include "Math/TMath.h"
#include "Framework/Str/RenderSurfaceType.h"

// 前方宣言
namespace Tsumi::Resource { class TextureAsset;  }

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
	SurfaceType surface{};

	// CPUで確定できる値
	Math::Vec4f color{ 1,1,1,1 };
	Math::Mat3x3 uvMat{};

	// テクスチャ
	Tsumi::Resource::TextureAsset* albedo = nullptr;

	bool visible = true;
};

// マテリアル情報
struct MaterialContext {
	std::unordered_map<MaterialKey, MaterialResolved, MaterialKeyHash> cache;

	void Clear() { cache.clear(); }

	const MaterialResolved* Find(const MaterialKey& k) const {
		auto it = cache.find(k);
		return (it != cache.end()) ? &it->second : nullptr;
	}
};

}