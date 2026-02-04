#pragma once

namespace Tsumi::Framework {

/* 更新順序 */
enum class UpdatePhase : uint8_t {
	// --------------------
	// Game Logic
	// --------------------
	PreLogic = 0,
	Logic,
	PostLogic,

	// --------------------
	// Scene Data Build (CPU)
	// --------------------
	Transform,
	SceneContext,
	RenderPrepare,

	// --------------------
	// Render Execution
	// --------------------
	RenderExecute,

	Count
};

}