#pragma once

#include "Math/TMath.h"
#include "../IComponent.h"

namespace Tsumi::Framework {

struct SRT {
	Math::Vec3f scale{};
	Math::Vec3f rotate{};
	Math::Vec3f translate{};
	SRT() :
		scale{ 1.0f, 1.0f, 1.0f },
		rotate{ 0.0f, 0.0f, 0.0f },
		translate{ 0.0f, 0.0f, 0.0f }
	{}
};

/* Actorの「位置・回転・スケール」を管理 */
class TransformComponent : public IComponent {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	TransformComponent() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~TransformComponent() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 親の設定
	/// </summary>
	void AttachToParent(std::weak_ptr<TransformComponent> parent);
	void DetachFromParent();

	/// <summary>
	/// 親がいるか
	/// </summary>
	bool HasParent() const {
		return !parent_.expired();
	}

#pragma region Accessor
	Math::Vec3f GetWorldPos() const { 
		return { worldMat_.m[3][0], worldMat_.m[3][1], worldMat_.m[3][2] }; }
	const Math::Mat4x4& GetWorldMatrix() const {
		return worldMat_; }
#pragma endregion 

private:
	/// <summary>
	/// 行列の更新
	/// </summary>
	void UpdateMat();

	/// <summary>
	/// 更新が必要か
	/// </summary>
	bool NeedsUpdate();

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void DrawImGui(std::string label = "");

public:
	// SRT
	SRT srt_{};

private:
	SRT prevSRT_{};
	// 行列
	Math::Mat4x4 worldMat_{};
	// 親子
	std::weak_ptr<TransformComponent> parent_;
	//std::vector<std::weak_ptr<TransformComponent>> children_;
};

}