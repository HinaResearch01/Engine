#include "TrasnformComponent.h"

using namespace Tsumi::Framework;
using namespace Tsumi::Math;

void TransformComponent::Init()
{
	// 行列はIdentityで初期化
	mat_.World.Identity();
	mat_.WVP.Identity();
	mat_.WorldInverseTranspose.Identity();
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

 Vec3f TransformComponent::GetWorldPos()
{
	 return { mat_.World.m[3][0], mat_.World.m[3][1], mat_.World.m[3][2] };
}

 const Mat4x4& TransformComponent::GetWorldMatrix()
 {
	UpdateMat();
	return mat_.World;
 }

 void TransformComponent::UpdateMat()
 {
	 // ローカル行列
	 mat_.World = 
		 Func::MAT4x4::AffineMatrix(srt_.scale, srt_.rotate, srt_.translate);

	 // 親のワールド行列を掛ける
	 if (auto parent = parent_.lock()) {
		 mat_.World *= parent->mat_.World;
	 }

	 // ワールド逆転置行列
	 mat_.WorldInverseTranspose = mat_.World.Inverse().Transpose();

	 // WVP 行列は描画時に cameraManager などから計算するのが良い

 }

 void TransformComponent::DrawImGui(std::string label)
{
	 label;
}
