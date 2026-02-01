#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "Math/TMath.h"
#include "Graphic/ShadowDMap/ShadowDepthMap.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Str/RenderPacket.h"
#include "Framework/Str/LightPacket.h"

namespace Tsumi {

namespace DX12 {
class DX12Manager;
class CommandContext;
struct FrameContext;
}

namespace Graphic {
class PSOLibrary;
class RootSignatureLibrary;
class ShadowDepthMap;
}

namespace Framework {

class World;
class ShadowSystem;
class CameraSystem;
class LightSystem;

/* 描画管理クラス */
class RenderSystem : public IUpdatable {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	explicit RenderSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RenderSystem() = default;

	/// <summary>
	/// Update（描画は RenderModel で行う）
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// Phase
	/// </summary>
	UpdatePhase Phase() const override { return UpdatePhase::RenderSys; }

	/// <summary>
	/// 描画エントリ
	/// </summary>
	void RenderBackSprite(DX12::CommandContext& cmd);
	void RenderModel(DX12::CommandContext& cmd);
	void RenderFrontSprite(DX12::CommandContext& cmd);

	/// <summary>
	/// リサイズ通知
	/// </summary>
	void OnResize(uint32_t w, uint32_t h);

private:
	// =========================================================
	// High-level render flow
	// =========================================================
	void DrawShadowPass(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame);

	void DrawGBufferPass(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame,
		const RenderPrepareSystem& prep);

	void DrawLightingPass(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame);

	void DrawDebugPass(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame);

	// =========================================================
	// Resource sync
	// =========================================================
	void SyncShadowResources();

	// =========================================================
	// Binding helpers（FrameContext 前提）
	// =========================================================
	void BindGBufferCamera(
		DX12::FrameContext& frame,
		const RenderPrepareSystem& prep);

	void BindGBufferObjects(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame,
		const RenderPrepareSystem& prep);

	void BindLightingCommon(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame);

	void BindDebugCommon(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame);

	// =========================================================
	// Draw helpers
	// =========================================================
	void DrawShadowCasters(
		DX12::CommandContext& cmd,
		DX12::FrameContext& frame);

private:
	// Shadow
	std::unique_ptr<Graphic::ShadowDepthMap> shadowDMap_;
	uint32_t cachedShadowSize_ = 0;

	// State
	int debugMode_ = 0;

	// References
	World& world_;
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Graphic::PSOLibrary* psoLib_ = nullptr;
	Graphic::RootSignatureLibrary* rsLib_ = nullptr;
};

} 
} 