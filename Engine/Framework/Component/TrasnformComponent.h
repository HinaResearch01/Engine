#pragma once

#include "Math/TMath.h"
#include "IComponent.h"

namespace Tsumi::Framework {

struct ObjMat {
	Math::Mat4x4 World{};
	Math::Mat4x4 WVP{};
	Math::Mat4x4 WorldInverseTranspose{};
};
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

#pragma region Accessor
	// ワールド座標
	Math::Vec3f GetWorldPos();
	// ワールド座標
	const Math::Mat4x4& GetWorldMatrix();
	// SRT
	void SetSRT(const SRT& srt) { srt_ = srt; }
	void SetScale(const Math::Vec3f& s) { srt_.scale = s; MarkDirty(); }
	void SetRotate(const Math::Vec3f& r) { srt_.rotate = r; MarkDirty(); }
	void SetTranslate(const Math::Vec3f& t) { srt_.translate = t; MarkDirty(); }
#pragma endregion 

private:
	/// <summary>
	/// 行列の更新
	/// </summary>
	void UpdateMat();

	/// <summary>
	/// 非静止
	/// </summary>
	void MarkDirty() { isDirty_ = true; }

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void DrawImGui(std::string label = "");

private:
	// SRT
	SRT srt_{};
	// 行列
	ObjMat mat_{};
	// 親子
	std::weak_ptr<TransformComponent> parent_;
	std::vector<std::weak_ptr<TransformComponent>> children_;
	bool isDirty_ = true; // SRT が変化したときに true にする
};
}