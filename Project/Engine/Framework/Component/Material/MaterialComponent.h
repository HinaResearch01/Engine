#pragma once

#include "Math/TMath.h"
#include "../IComponent.h"

using TextureHandle = std::string;

namespace Tsumi::Framework {

/* マテリアル情報を管理するコンポーネント */
class MaterialComponent : public IComponent {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	MaterialComponent() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~MaterialComponent() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init() override {}

public:
	// ベースカラー
	Math::Vec4f baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	// アルベドテクスチャハンドル
	TextureHandle texKey_ = "uvChecker.png";
};

}