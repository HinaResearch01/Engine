#pragma once

namespace Tsumi::Framework {

/* 更新順序 */
enum class UpdatePhase : uint8_t {
	PreLogic = 0,   // 入力反映・事前準備
	Logic,          // ゲームロジック
	PostLogic,      // 後処理・整合性
	Camera,         // カメラ評価
	RenderPrepare,  // 描画準備
	Count
};

}