#pragma once

#include <cstdint>

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
	// Debug / UI
	// --------------------
	DebugUI,

	// --------------------
	// Render Execution
	// --------------------
	RenderExecute,

	Count
};

}