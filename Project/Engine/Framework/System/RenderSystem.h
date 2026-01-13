#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Render/RenderPacket.h"
#include "Framework/Render/RenderSurfaceType.h"

// 前方宣言
namespace Tsumi::DX12 {
class DX12Manager;
class CommandContext;
}
namespace Tsumi::Graphic {
class PSOLibrary;
class RootSignatureLibrary;
}
namespace Tsumi::Resource {
class ResourceSystem;
struct MeshAsset;
}

namespace Tsumi::Framework {

class World;
class CameraSystem;
class MaterialSystem;

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
	void RenderBackSprite(DX12::CommandContext& cmd);
	void RenderModel(DX12::CommandContext& cmd);
	void RenderFrontSprite(DX12::CommandContext& cmd);

#pragma region Accessor

#pragma	endregion

private:
	/// <summary>
	/// 描画リストのソート
	/// </summary>
	void SortLists();

private:
	using List = std::vector<DrawPacket>;
	std::array<List, static_cast<size_t>(SurfaceType::Count)> lists_;
	
	World& world_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Resource::ResourceSystem* resourceSys_ = nullptr;
	Graphic::PSOLibrary* psoLib_ = nullptr;
	Graphic::RootSignatureLibrary* rootSigLib_ = nullptr;
};

}