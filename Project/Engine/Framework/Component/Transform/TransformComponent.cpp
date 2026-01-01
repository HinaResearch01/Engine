#include "TransformComponent.h"

using namespace Tsumi::Framework;
using namespace Tsumi::Math;

void TransformComponent::Init()
{
	// 行列はIdentityで初期化
	worldMat_.Identity();
	prevSRT_ = srt; // 初期状態を記録
}

void TransformComponent::Update()
{
	// 親がいる場合は毎回更新
	if (HasParent() || NeedsUpdate()) {
		UpdateMat();
		prevSRT_ = srt;
	}
}

void TransformComponent::AttachToParent(std::weak_ptr<TransformComponent> parent)
{
	// TODO : 機能として不十分
	parent_ = parent;
}

void TransformComponent::DetachFromParent()
{
	// TODO : 機能として不十分
	parent_.reset();
}

void TransformComponent::UpdateMat()
{
	// ローカル行列
	Math::Mat4x4 local =
		Func::MAT4x4::AffineMatrix(
		srt.scale, srt.rotate, srt.translate);

	// 親がいる場合は親のワールド行列を掛ける
	if (auto parent = parent_.lock()) {
		worldMat_ = local * parent->worldMat_;
	}
	else {
		worldMat_ = local;
	}
}

bool TransformComponent::NeedsUpdate()
{
	if (srt.scale != prevSRT_.scale)     return true;
	if (srt.rotate != prevSRT_.rotate)    return true;
	if (srt.translate != prevSRT_.translate) return true;
	if (parent_.expired() == false)           return true; // 親ありは毎回
	return false;
}

void TransformComponent::DrawImGui(std::string label)
{
	label;
}
