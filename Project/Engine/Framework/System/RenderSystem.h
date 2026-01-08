#pragma once

#include "Math/TMath.h"
#include "Framework/Render/RenderStructure.h"
#include "Framework/Update/IUpdatable.h"

#include <cstdint>

// 前方宣言
namespace Tsumi::DX12 {
class DX12Manager;
}
namespace Tsumi::Graphic {
class PSOLibrary;
class RootSignatureLibrary;
}
namespace Tsumi::Resource {
class ResourceSystem;
}

namespace Tsumi::Framework {

class World;

/* 描画管理クラス */
class RenderSystem : public IUpdatable {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	RenderSystem() = default;
	RenderSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RenderSystem() = default;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phaseの取得
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::Render; }

	/// <summary>
	/// 描画処理
	/// </summary>
	void Render();

#pragma region Accessor

#pragma	endregion

private:

private:
	std::vector<RenderItem> items_;
	
	World& world_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Resource::ResourceSystem* resourceSys_ = nullptr;
	Graphic::PSOLibrary* psoLib_ = nullptr;
	Graphic::RootSignatureLibrary* rootSigLib_ = nullptr;
};

}