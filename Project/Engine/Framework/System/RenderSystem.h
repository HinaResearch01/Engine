#pragma once

#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"

#include <cstdint>

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
	/// queueの構築
	/// </summary>
	void BuildRenderQueue();

	/// <summary>
	/// queueのソート
	/// </summary>
	void SortRenderQueue();

	/// <summary>
	/// ソートキーの作成
	/// </summary>
	/*int64_t MakeSortKey(RenderQueue q, const Material* m, const Resource::MeshAsset* mesh)
	{
		uint64_t key = 0;
		key |= (uint64_t(q) & 0xFFFF) << 48;
		key |= (uint64_t(m) & 0xFFFFFFFF) << 16;
		key |= (uint64_t(mesh) & 0xFFFF);
		return key;
	}*/

private:
	
	World& world_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Resource::ResourceSystem* resourceSys_ = nullptr;
	Graphic::PSOLibrary* psoLib_ = nullptr;
	Graphic::RootSignatureLibrary* rootSigLib_ = nullptr;
};

}