#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "Math/TMath.h"
#include "Graphic/ShadowDMap/ShadowDepthMap.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Str/RenderPacket.h"
#include "Framework/Str/LightPacket.h"

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
class TransformComponent;

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

private:
	/// <summary>
	/// Pass
	/// </summary>
	void GBufferPass(const std::array<std::vector<RenderPacket>, static_cast<size_t>(SurfaceType::Count)>& lists);
	void LightingPass(const LightPacket& lightPacket);

	/// <summary>
	/// 各パケットの描画処理
	/// </summary>
	void RenderPackets(DX12::CommandContext& cmd, const RenderPacket& pkt);

	/// <summary>
	/// バインド処理
	/// </summary>
	void BindMesh(DX12::CommandContext& cmd, const RenderPacket& pkt);
	void BindTransform(DX12::CommandContext& cmd, const RenderPacket& pkt);
	void BindMaterial(DX12::CommandContext& cmd, const RenderPacket& pkt);

	/// <summary>
	/// 描画コマンド
	/// </summary>
	void DrawCommand(DX12::CommandContext& cmd, const RenderPacket& pkt);

private:
	std::unique_ptr<Graphic::ShadowDepthMap> shadowDMap_;

	World& world_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Resource::ResourceSystem* resourceSys_ = nullptr;
	Graphic::PSOLibrary* psoLib_ = nullptr;
	Graphic::RootSignatureLibrary* rootSigLib_ = nullptr;
};

}