#pragma once
#pragma once

#include <unordered_map>
#include <string>
#include "Math/TMath.h"
#include "Resource/Tex/TextureManager.h"

namespace Tsumi::Framework {

// 前方宣言
class RenderMaterial;

/*  */
class MaterialInstance {

public:
	// コンストラクタ
	MaterialInstance() = default;

	// 取得
	Resource::TextureAsset* GetTexture(const std::string& name) const {
		auto it = textures.find(name);
		return it != textures.end() ? it->second : nullptr;
	}

	// 設定
	void SetTexture(const std::string& name, Resource::TextureAsset* tex) {
		textures[name] = tex;
	}

public:
	RenderMaterial* parent = nullptr;

private:
	std::unordered_map<std::string, Resource::TextureAsset*> textures;
};


}