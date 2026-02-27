#include "CameraSystem.h"
#include "Framework/World/World.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include <limits>
#undef min
#undef max

using namespace Tsumi::Framework;
using namespace tme;

CameraSystem::CameraSystem(World& world)
	: ISystem(world)
{}

void CameraSystem::Init()
{
	BuildDefault(defaultCtx_);
	activeCtx_ = defaultCtx_;
}

void CameraSystem::Update(float)
{
	if (IActor* camActor = SelectCamera()) {
		BuildFromActor(camActor, activeCtx_);
	}
	else {
		activeCtx_ = defaultCtx_;
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

	float aspect = world_.GetAspectRatio();

	out.fovY = math::func::NUM::ToRadians(cam->fovY);
	out.aspectRatio = aspect;
	out.nearPlane = cam->nearZ;
	out.farPlane = cam->farZ;

	// ----------------------------
	// Worldから取得
	// ----------------------------
	const math::Mat4x4& camWorld = tr->world;

	const math::Vec3f position = camWorld.GetPosition();
	const math::Vec3f forward = camWorld.GetForward().Normalized();
	const math::Vec3f up = camWorld.GetUp().Normalized();

	const math::Vec3f target = position + forward;

	out.position = position;
	out.forward = forward;

	// ----------------------------
	// View
	// ----------------------------
	out.view = math::func::MAT4x4::LookAtLH(position, target, up);

	// ----------------------------
	// Projection
	// ----------------------------
	out.proj = math::func::MAT4x4::PerspectiveFovMatrix(
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
	const float defFov = 60.0f;
	const float defNear = 0.1f;
	const float defFar = 1000.0f;

	const math::Vec3f rotate{ -30, 0,0 };
	const math::Vec3f position = { 0,45,-60 };

	out.fovY = math::func::NUM::ToRadians(defFov);
	out.aspectRatio = world_.GetAspectRatio();
	out.nearPlane = defNear;
	out.farPlane = defFar;

	// ----------------------------
	// Forward計算
	// ----------------------------
	const math::Mat4x4 rotMat = math::Mat4x4::Rotation(
		math::func::VEC3::ToRadians(rotate));

	const math::Vec3f forward = rotMat.GetForward().Normalized();

	const math::Vec3f target = position + forward;
	const math::Vec3f up{ 0,1,0 };

	out.position = position;
	out.forward = forward;

	// ----------------------------
	// View
	// ----------------------------
	out.view = math::func::MAT4x4::LookAtLH(position, target, up);

	// ----------------------------
	// Projection
	// ----------------------------
	out.proj = math::func::MAT4x4::PerspectiveFovMatrix(
		out.fovY,
		out.aspectRatio,
		out.nearPlane,
		out.farPlane
	);

	out.viewProj = MakeViewProj(out.view, out.proj);
	out.invView = out.view.Inverse();
	out.invProj = out.proj.Inverse();
	out.invViewProj = out.viewProj.Inverse();

	out.valid = true;
}

tme::math::Mat4x4 CameraSystem::MakeViewProj(const math::Mat4x4& view, const math::Mat4x4& proj)
{
	return view * proj;
}
