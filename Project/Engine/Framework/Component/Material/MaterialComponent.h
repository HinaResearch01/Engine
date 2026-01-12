#pragma once

#include "Math/TMath.h"
#include "../IComponent.h"
#include "Framework/Render/rendermanteri"

using TextureHandle = std::string;

namespace Tsumi::Framework {

/* マテリアル情報を管理するコンポーネント */
class MaterialComponent : public IComponent {

public:
	// コンストラクタ
	MaterialComponent() : IComponent("MaterialComponent") {}

	// デストラクタ
	~MaterialComponent() = default;

	void SetMaterial(const std::string& alias) {
		if (matKey_ != alias) {
			matKey_ = alias;
			isDirty_ = true;
			cachedMat_ = nullptr;
		}
	}

	void ResolveMaterial(RenderMaterial* ptr) { // shared_ptrの場合は適宜変更
		cachedMat_ = ptr;
		isDirty_ = false;
	}

	RenderMaterial* GetResolvedMaterial() const {
		return cachedMat_;
	}

	bool IsDirty() const { return isDirty_; }
	std::string GetKey() const { return matKey_; }

private:
	std::string matKey_ = "";
	RenderMaterial* cachedMat_ = nullptr;
	bool isDirty_ = false;
};

}