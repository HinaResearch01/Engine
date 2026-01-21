#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "Math/TMath.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/Str/RenderGpuPackets.h"
#include "Framework/Str/RenderPacket.h"
#include "Framework/Str/RenderPassTable.h"
#include "Framework/Str/RenderSurfaceType.h"

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
	UpdatePhase Phase() const override { return UpdatePhase::RenderPass; }

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
	/// パケットの組み立て
	/// </summary>
	void BuildDrawPackets();

	/// <summary>
	/// Transformの行列計算
	/// </summary>
	void FillTransformPacket(DrawPacket& pkt, const TransformComponent& tc);

	/// <summary>
	/// 描画リストのクリア
	/// </summary>
	void ClearLists();

	/// <summary>
	/// 描画リストのソート
	/// </summary>
	void SortLists();

	/// <summary>
	/// ViewCB アップロード
	/// </summary>
	D3D12_GPU_VIRTUAL_ADDRESS UploadViewCB();

	/// <summary>
	/// Pass 単位描画
	/// </summary>
	void RenderSurfacePass(
		DX12::CommandContext& cmd,
		SurfaceType surface,
		D3D12_GPU_VIRTUAL_ADDRESS viewCBAddr);

	/// <summary>
	/// RenderPassDescの設定
	/// </summary>
	void SetupPassState(
		DX12::CommandContext& cmd,
		const RenderPassDesc& pass,
		D3D12_GPU_VIRTUAL_ADDRESS viewCBAddr);

	/// <summary>
	/// 各パケットの描画処理
	/// </summary>
	void RenderPacket(DX12::CommandContext& cmd, const DrawPacket& pkt);

	/// <summary>
	/// バインド処理
	/// </summary>
	void BindCamera();
	void BindMesh(DX12::CommandContext& cmd, const DrawPacket& pkt);
	void BindTransform(DX12::CommandContext& cmd, const DrawPacket& pkt);
	void BindMaterial(DX12::CommandContext& cmd, const DrawPacket& pkt);

	/// <summary>
	/// 描画コマンド
	/// </summary>
	void DrawCommand(DX12::CommandContext& cmd, const DrawPacket& pkt);

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