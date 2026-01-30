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
	UpdatePhase Phase() const override { return UpdatePhase::RenderSys; }

	/// <summary>
	/// 描画処理
	/// </summary>
	void RenderBackSprite(DX12::CommandContext& cmd);
	void RenderModel(DX12::CommandContext& cmd);
	void RenderFrontSprite(DX12::CommandContext& cmd);

	void OnResize(uint32_t w, uint32_t h);

private:
	// ---------------------------------------------------------
	// High-level render flow
	// ---------------------------------------------------------
	void DrawShadowPass(DX12::CommandContext* cmd);
	void DrawGBufferPass(DX12::CommandContext* cmd);
	void DrawLightingPass(DX12::CommandContext* cmd);
	void DrawDebugPass(DX12::CommandContext* cmd);

	// ---------------------------------------------------------
	// Resource sync
	// ---------------------------------------------------------
	void SyncShadowResources();     // ShadowDepthMap / DSV / SRV

	// ---------------------------------------------------------
	// Binding helpers
	// ---------------------------------------------------------
	void BindGBufferCommon(DX12::CommandContext* cmd);
	void BindLightingCommon(DX12::CommandContext* cmd);
	void BindDebugCommon(DX12::CommandContext* cmd);

	// ---------------------------------------------------------
	// Draw helpers
	// ---------------------------------------------------------
	void DrawShadowCasters(DX12::CommandContext* cmd);
	void DrawGBufferObjects(DX12::CommandContext* cmd);

private:
	std::unique_ptr<Graphic::ShadowDepthMap> shadowDMap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> shadowDsvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE shadowDsv_{};
	int debugMode_ = 0;
	uint32_t cachedShadowSize_ = 0;

	World& world_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Resource::ResourceSystem* resourceSys_ = nullptr;
	Graphic::PSOLibrary* psoLib_ = nullptr;
	Graphic::RootSignatureLibrary* rsLib_ = nullptr;
};

}