#pragma once
#include "Math/TMath.h"
#include <cstdint>

// 前方宣言
namespace Tsumi::Resource {
struct MeshAsset;
}
namespace Tsumi::Framework {
class RenderMaterial;
}

namespace Tsumi::Framework {

// 描画レイヤー
enum class RenderLayer : uint8_t {
	Background = 0,
	Opaque = 1,
	AlphaTest = 2,
	Translucent = 3,
	Foreground = 4,
	UI = 5,
};

// 描画アイテム（System内部で使用）
struct RenderItem {
	// 解決済みのリソースポインタ
	Tsumi::Resource::MeshAsset* mesh = nullptr;
	RenderMaterial* material = nullptr;

	// インスタンス情報
	Math::Mat4x4 worldMatrix;
	Math::Vec4f color;

	// ソート用
	uint64_t sortKey = 0;
};

}