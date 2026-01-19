#include "CameraSystem.h"
#include "Win/Win32Window.h"
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
	win32_ = Win32::Win32Window::GetInstance();
	BuildDefault(activeCtx_);
}

void CameraSystem::Update(float)
{
	// 毎フレーム default を作る
	BuildDefault(activeCtx_);

	// カメラActorがあれば上書き
	if (IActor* camActor = SelectCamera()) {
		BuildFromActor(camActor, activeCtx_);
	}
}

IActor* CameraSystem::SelectCamera() const
{
	IActor* best = nullptr;
	int bestPriority = std::numeric_limits<int>::min();

	const auto& cameras = world_.GetCamerasCompView().GetActors();
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

	Math::Vec3f s{ 1,1,1 };
	Math::Vec3f r = tr->srt.rotate;
	Math::Vec3f t = tr->GetWorldPos();

	out.position = t;

	Math::Mat4x4 camWorld =
		Math::Func::MAT4x4::AffineMatrix(s, r, t);

	out.view = camWorld.Inverse();

	const float aspect = float(win32_->GetAspectRatio());

	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		Math::Func::NUM::ToRadians(cam->fovY),
		aspect,
		cam->nearZ,
		cam->farZ
	);

	out.viewProj = out.proj * out.view;
	out.valid = true;
}

void CameraSystem::BuildDefault(CameraContext& out)
{
	Math::Vec3f scale{ 1,1,1 };
	Math::Vec3f rotate{ 0,0,0 };
	Math::Vec3f translate{ 0,2,-6 };

	Math::Mat4x4 camWorld =
		Math::Func::MAT4x4::AffineMatrix(scale, rotate, translate);

	out.view = camWorld.Inverse();

	const float aspect = float(win32_->GetAspectRatio());

	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		Math::Func::NUM::ToRadians(60.0f),
		aspect,
		0.1f,
		1000.0f
	);

	out.viewProj = out.proj * out.view;
	out.position = translate;
	out.valid = true;
}
