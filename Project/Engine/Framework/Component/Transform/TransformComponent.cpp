#include "TransformComponent.h"

using namespace Tsumi::Framework;
using namespace Tsumi::Math;

void TransformComponent::Init()
{
	// 行列はIdentityで初期化
	worldMat_.Identity();
}

void TransformComponent::Update()
{
	// Dirty フラグが立っている場合のみ行列を再計算
	if (isDirty_ || (parent_.lock() && parent_.lock()->isDirty_)) {
		UpdateMat();
		isDirty_ = false;
	}
}

void TransformComponent::AttachToParent(std::weak_ptr<TransformComponent> parent)
{
	// TODO : 機能として不十分
	parent_ = parent;
	MarkDirty();
}

void TransformComponent::DetachFromParent()
{
	// TODO : 機能として不十分
	parent_.reset();
	MarkDirty();
}

void TransformComponent::UpdateMat()
{
	// ローカル行列
	worldMat_ =
		Func::MAT4x4::AffineMatrix(srt_.scale, srt_.rotate, srt_.translate);

	// 親のワールド行列を掛ける
	if (auto parent = parent_.lock()) {
		worldMat_ *= parent->worldMat_;
	}
}

void TransformComponent::DrawImGui(std::string label)
{
	label;
}
