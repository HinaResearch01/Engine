#include "TransformSystem.h"
#include "Framework/World/World.h"

using namespace Tsumi::Framework;

TransformSystem::TransformSystem(World& world)
	: world_(world)
{}

void TransformSystem::Update(float)
{
	auto& view = world_.GetTransforms();
	const auto& actors = view.GetActors();

	// ルート（親なし）から更新
	for (IActor* a : actors) {
		auto* tr = a->GetComponent<TransformComponent>();
		if (!tr) continue;
		if (!tr->parent.expired()) continue;

		UpdateComponent(*tr);
	}

	// 子（親あり）を更新
	// 深い階層がある場合でも、親が先に更新されていればOK
	for (IActor* a : actors) {
		auto* tr = a->GetComponent<TransformComponent>();
		if (!tr) continue;
		if (tr->parent.expired()) continue;

		UpdateComponent(*tr);
	}
}

void TransformSystem::UpdateComponent(TransformComponent& tr)
{
	const bool selfDirty = tr.IsSelfDirty();
	if (!selfDirty && !tr.parentDirty) {
		return; // 完全に静的
	}

	// ==================================
	// 1. ローカル行列生成
	// ==================================
	const Math::Mat4x4 localMat =
		Math::Mat4x4::Scale(tr.srt.scale) *
		Math::Mat4x4::Rotation(tr.srt.rotate) *
		Math::Mat4x4::Translation(tr.srt.translate);

	// ==================================
	// 2. ワールド行列
	// ==================================
	if (auto p = tr.parent.lock()) {
		tr.world = p->world * localMat;
		tr.parentDirty = p->IsSelfDirty() || p->parentDirty;
	}
	else {
		tr.world = localMat;
		tr.parentDirty = false;
	}

	// ==================================
	// 3. right / up / forward 更新
	// ==================================
	// 行列の回転部分（スケール込み）を抽出
	Math::Vec3f r{
		tr.world.m[0][0],
		tr.world.m[0][1],
		tr.world.m[0][2]
	};

	Math::Vec3f u{
		tr.world.m[1][0],
		tr.world.m[1][1],
		tr.world.m[1][2]
	};

	Math::Vec3f f{
		tr.world.m[2][0],
		tr.world.m[2][1],
		tr.world.m[2][2]
	};

	// 非一様スケール対策：正規直交化
	r.Normalize();
	u = (u - r * Math::Func::VEC3::Dot(u, r)).Normalize();
	f = Math::Func::VEC3::Cross(r, u).Normalize();

	tr.right = r;
	tr.up = u;
	tr.forward = f;

	// ==================================
	// 4. prev 同期
	// ==================================
	tr.SyncPrev();
}
