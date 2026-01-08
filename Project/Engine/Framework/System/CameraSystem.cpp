#include "CameraSystem.h"
#include "Framework/World/World.h"
#include "Framework/Actor/IActor.h"
#include "Utils/Logger/Logger.h"
#include <limits>
#undef min
#undef max

using namespace Tsumi::Framework;

CameraSystem::CameraSystem(World& world)
	: world_(world)
{
	BuildDefault(defaultCtx_);
	activeCtx_ = defaultCtx_;
}

void CameraSystem::Update(float)
{
	// デフォルトをまず採用（カメラ無しでも描画可）
	activeCtx_ = defaultCtx_;

	IActor* camActor = SelectCamera();
	if (!camActor) {
		return;
	}

	BuildFromActor(camActor, activeCtx_);
}

IActor* CameraSystem::SelectCamera() const
{
	IActor* best = nullptr;
	int bestPriority = std::numeric_limits<int>::min();

	const auto& cameras = world_.GetCameras().GetActors();
	for (IActor* actor : cameras) {
		auto* cam = actor->GetComponent<CameraComponent>();
		if (!cam) continue;
		if (!cam->active) continue;
		if (!cam->mainCandidate) continue;

		if (cam->priority > bestPriority) {
			bestPriority = cam->priority;
			best = actor;
		}
	}

	return best;
}

void CameraSystem::BuildFromActor(IActor* actor, CameraContext& out)
{
	auto* tr = actor->GetComponent<TransformComponent>();
	auto* cam = actor->GetComponent<CameraComponent>();
	if (!tr || !cam) return;

	out.valid = true;

	const Math::Vec3f s = Math::Vec3f{ 1.0f, 1.0f, 1.0f };
	const Math::Vec3f r = tr->srt.rotate;
	const Math::Vec3f t = tr->GetWorldPos();

	out.position = t;

	out.view = Math::Func::MAT4x4::AffineMatrix(s, r, t);
	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		Math::Func::NUM::ToRadians(cam->fovY),
		cam->aspect,
		cam->nearZ,
		cam->farZ
	);
	out.viewProj = out.view * out.proj;
}

void CameraSystem::BuildDefault(CameraContext& out)
{
	Math::Vec3f scale{ 1,1,1 };
	Math::Vec3f rotate{ 0,0,0 };
	Math::Vec3f translate{ 0,2,-6 };

	out.view = Math::Func::MAT4x4::AffineMatrix(
		scale,
		-rotate,
		-translate
	);

	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		Math::Func::NUM::ToRadians(60.0f),
		16.0f / 9.0f,
		0.1f,
		1000.0f
	);

	out.viewProj = out.view * out.proj;
	out.position = translate;
	out.valid = true;
}
