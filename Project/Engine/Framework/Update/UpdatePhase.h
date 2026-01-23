#pragma once

namespace Tsumi::Framework {

/* 更新順序 */
enum class UpdatePhase : uint8_t {
	// --------------------
	// Logic / Simulation
	// --------------------
	PreLogic = 0,   // 入力反映、外部イベント
	Logic,          // ゲームロジック
	PostLogic,      // 整合性調整

	// --------------------
	// Render Preparation (CPU)
	// --------------------
	TransformSys,      // World行列確定
	CameraSys,         // CameraContext構築
	MaterialSys,       // MaterialInstance評価
	LightSys,          // LightContext構築
	ShadowSys,         // ShadowContext構築
	RenderPrepareSys,  // DrawPacket / GBufferPacket生成

	// --------------------
	// Render Execution (GPU)
	// --------------------
	RenderSys,         // ShadowPass / GBuffer / Lighting / Debug

	Count
};

}