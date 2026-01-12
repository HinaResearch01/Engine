#pragma once

#include <cstdint>

namespace Tsumi::Framework {

enum class RenderQueue : uint16_t {
	Opaque = 2000,
	AlphaTest = 2450,
	Transparent = 3000,
};

enum class RenderLayer : uint8_t {
	Background = 0, // 背景2D (BackSprite)
	Opaque = 1, // 不透明3D (Model)
	AlphaTest = 2, // アルファテスト (草木など)
	Translucent = 3, // 半透明3D (未実装なら予約)
	Foreground = 4, // 前景2D (FrontSprite)
	UI = 5, // ImGuiなど (今回は別枠だが予約)
	Count
};

union RenderSortKey {
	uint64_t value;
	struct {
		uint64_t depth : 32; // 深度 (3Dならカメラ距離, 2DならZオーダー)
		uint64_t material : 16; // マテリアルID or テクスチャID (ステート変更抑制)
		uint64_t pso : 12; // PSO ID (最も重い切り替えなので上位に)
		uint64_t layer : 4;  // RenderLayer (最上位ビット)
	} fields;

	// ソート用比較演算子
	bool operator<(const RenderSortKey& other) const {
		// 基本は昇順 (小さい方が先)
		return value < other.value;
	}
};

}