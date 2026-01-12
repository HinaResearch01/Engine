#pragma once

#include "../IComponent.h"
#include "Framework/Material/MaterialInstance.h"

namespace Tsumi::Resource { 
struct MeshAsset;
}

namespace Tsumi::Framework {

/* Actorの「描画」を管理　データコンポネント */
class RenderComponent : public IComponent {

public:
	// コンストラクタ
	RenderComponent() : IComponent("RenderComponent") {}

	// デストラクタ
	~RenderComponent() = default;

	void SetMesh(Resource::MeshAsset* mesh) { mesh_ = mesh; }
	void SetMaterial(const std::string& alias) {
		alias;
		//material_ = MaterialSystem::Get().Acquire(alias);
	}

	Resource::MeshAsset* GetMesh() const { return mesh_; }
	MaterialInstance* GetMaterial() const { return material_; }


private:
	Resource::MeshAsset* mesh_ = nullptr;
	MaterialInstance* material_ = nullptr;
};

}