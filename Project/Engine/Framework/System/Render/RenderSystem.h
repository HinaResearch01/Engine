#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "Math/TMath.h"
#include "../ISystem.h"
#include "Graphic/ShadowDMap/ShadowDepthMap.h"
#include "Graphic/ShadowDMap/CSMShadowDepthMap.h"
#include "Framework/Str/RenderPacket.h"
#include "Framework/Str/LightPacket.h"

namespace Tsumi {

namespace DX12 {
class DX12Manager;
class CommandContext;
class FrameResources;
}

namespace Graphic {
class PSOLibrary;
class RootSignatureLibrary;
class ShadowDepthMap;
}

namespace Framework {

class World;
class CameraSystem;
class LightSystem;
class ShadowSystem;
class RenderPrepareSystem;

/* 描画管理クラス */
class RenderSystem : public ISystem {

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
	UpdatePhase Phase() const override { return UpdatePhase::RenderExecute; }

	/// <summary>
	/// 描画パスエントリ
	/// </summary>
	void DrawShadowPass(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void DrawGBufferPass(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void DrawLightingPass(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void DrawDirectionalLights(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void DrawPointLights(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void DrawSpotLights(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void DrawDebugPass(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame);

	/// <summary>
	/// リサイズ通知
	/// </summary>
	void OnResize(uint32_t w, uint32_t h);

private:
	// =========================================================
	// Resource sync
	// =========================================================
	void SyncShadowResources();

	// =========================================================
	// Binding helpers（FrameContext 前提）
	// =========================================================
	void BindGBufferCamera(
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void BindGBufferObjects(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void BindDirectionalLighting(
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);
	void BindDebug(
		DX12::FrameResources& frame);
	void BindShadow(
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep,
		uint32_t cascadeIndex);

	// =========================================================
	// Draw helpers
	// =========================================================
	void DrawShadowCasters(
		DX12::CommandContext& cmd,
		DX12::FrameResources& frame,
		const RenderPrepareSystem& prep);

private:
	// Shadow
	std::unique_ptr<Graphic::CSMShadowDepthMap> shadowDMap_;
	uint32_t cachedShadowSize_ = 0;
	uint32_t cachedCascadeCount_ = 0;

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