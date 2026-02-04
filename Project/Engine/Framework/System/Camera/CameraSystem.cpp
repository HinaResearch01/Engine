#include "CameraSystem.h"
#include "Framework/World/World.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include <limits>
#undef min
#undef max

using namespace Tsumi::Framework;

CameraSystem::CameraSystem(World& world)
	: world_(world)
{
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

	// ----------------------------
	// Aspect ratio
	// ----------------------------
	float aspect = world_.GetAspectRatio();

	// ----------------------------
	// Camera params
	// ----------------------------
	out.fovY = Math::Func::NUM::ToRadians(cam->fovY);
	out.aspectRatio = aspect;
	out.nearPlane = cam->nearZ;
	out.farPlane = cam->farZ;
	// TransformSystem成果物を使用
	const Math::Mat4x4& camWorld = tr->world;
	out.position = tr->GetWorldPos();
	out.forward = tr->forward;

	// ----------------------------
	// View / Projection
	// ----------------------------
	out.view = camWorld.Inverse();
	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		out.fovY, out.aspectRatio, out.nearPlane, out.farPlane
	);

	out.viewProj = MakeViewProj(out.view, out.proj);
	out.invView = out.view.Inverse();
	out.invProj = out.proj.Inverse();
	out.invViewProj = out.viewProj.Inverse();
	out.valid = true;
}

void CameraSystem::BuildDefault(CameraContext& out)
{
	// デフォルトは数値固定
	const float defFov = 60.0f;
	const float defNear = 0.1f;
	const float defFar = 1000.0f;

	out.fovY = Math::Func::NUM::ToRadians(defFov);
	out.aspectRatio = world_.GetAspectRatio();
	out.nearPlane = defNear;
	out.farPlane = defFar;

	out.position = Math::Vec3f{ 0,0,-25 };
	out.forward = Math::Vec3f{ 0,0, 1 };

	const Math::Vec3f scale{ 1,1,1 };
	const Math::Vec3f rotate{ 0,0,0 };
	const Math::Vec3f translate = out.position;

	const Math::Mat4x4 camWorld =
		Math::Func::MAT4x4::AffineMatrix(scale, rotate, translate);

	out.view = camWorld.Inverse();
	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		out.fovY, out.aspectRatio, out.nearPlane, out.farPlane
	);

	out.viewProj = MakeViewProj(out.view, out.proj);
	out.invView = out.view.Inverse();
	out.invProj = out.proj.Inverse();
	out.invViewProj = out.viewProj.Inverse();
	out.valid = true;
}

Tsumi::Math::Mat4x4 CameraSystem::MakeViewProj(const Math::Mat4x4& view, const Math::Mat4x4& proj)
{
	// ここで座標系を固定する
	return view * proj;
}
