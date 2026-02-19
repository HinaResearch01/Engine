#include "TransformSystem.h"
#include "Framework/World/World.h"

using namespace Tsumi::Framework;

TransformSystem::TransformSystem(World& world)
	: ISystem(world)
{}

void TransformSystem::Update(float)
{
	auto& view = world_.GetTransformsCompView();
	const auto& actors = view.GetActors();

	// 1. ルートを集める
	std::vector<TransformComponent*> roots;
	roots.reserve(actors.size());

	for (IActor* a : actors) {
		auto* tr = a->GetComponent<TransformComponent>();
		if (!tr) continue;
		if (tr->parent == nullptr) {
			roots.push_back(tr);
		}
	}

	// 2. ルートから順に更新
	for (TransformComponent* root : roots) {
		UpdateHierarchy(*root);
	}
}

void TransformSystem::UpdateHierarchy(TransformComponent& tr)
{
	// 自分を更新
	UpdateComponent(tr);

	// 子を更新
	auto& view = world_.GetTransformsCompView();
	for (IActor* a : view.GetActors()) {
		auto* child = a->GetComponent<TransformComponent>();
		if (!child) continue;
		if (child->parent == &tr) {
			UpdateHierarchy(*child);
		}
	}
}

void TransformSystem::UpdateComponent(TransformComponent& tr)
{
	// 親のdirtyを伝播
	if (tr.parent && tr.parent->worldDirty) {
		tr.worldDirty = true;
	}
	if (!tr.worldDirty) return;

	// 1) local
	const Math::Mat4x4 localMat =
		Math::Func::MAT4x4::AffineMatrix(
		tr.srt.scale,
		Math::Func::VEC3::ToRadians(tr.srt.rotate),
		tr.srt.translate
		);

	// 2) world
	if (tr.parent) {
		tr.world = tr.parent->world * localMat;
	}
	else {
		tr.world = localMat;
	}
	tr.worldInvTranspose = tr.world.Inverse().Transpose();

	// 3) basis（正規直交化）
	Math::Vec3f r{ tr.world.m[0][0], tr.world.m[0][1], tr.world.m[0][2] };
	Math::Vec3f u{ tr.world.m[1][0], tr.world.m[1][1], tr.world.m[1][2] };
	Math::Vec3f f{ tr.world.m[2][0], tr.world.m[2][1], tr.world.m[2][2] };

	r = r.Normalized();
	u = (u - r * Math::Func::VEC3::Dot(u, r)).Normalized();
	f = Math::Func::VEC3::Cross(r, u);

	tr.right = r;
	tr.up = u;
	tr.forward = f.Normalized();

	// 4) clear dirty
	tr.selfDirty = false;
	tr.worldDirty = false;
}