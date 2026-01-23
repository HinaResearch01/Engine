#include "CameraSystem.h"
#include "Win/Win32Window.h"
#include "DX12/DX12Manager.h"
#include "DX12/Framebuf/Framebuffer.h"
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
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
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
	auto fb = dx12Mgr_->GetFramebuffer();
	float width = static_cast<float>(fb->GetWidth());
	float height = static_cast<float>(fb->GetHeight());
	float aspect = (height > 0.0f) ? (width / height) : (1280.0f / 720.0f);

	// ----------------------------
	// Camera params
	// ----------------------------
	out.fovY = Math::Func::NUM::ToRadians(cam->fovY);
	out.aspectRatio = aspect;
	out.nearPlane = cam->nearZ;
	out.farPlane = cam->farZ;

	// ----------------------------
	// World transform
	// ----------------------------
	const Math::Vec3f scale{ 1,1,1 };
	const Math::Vec3f rotate = Math::Func::VEC3::ToRadians(tr->srt.rotate);
	const Math::Vec3f translate = tr->GetWorldPos();

	Math::Mat4x4 camWorld =
		Math::Func::MAT4x4::AffineMatrix(scale, rotate, translate);

	// ----------------------------
	// View / Projection
	// ----------------------------
	out.view = camWorld.Inverse();

	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		out.fovY,
		out.aspectRatio,
		out.nearPlane,
		out.farPlane
	);

	out.viewProj = out.view * out.proj;

	// ----------------------------
	// Derived vectors
	// ----------------------------
	out.position = translate;

	// +Z forward
	out.forward = 
		camWorld.TransformVector(Math::Vec3f{ 0,0,1 }).Normalized();

	out.valid = true;
}

void CameraSystem::BuildDefault(CameraContext& out)
{
	// デフォルトは数値固定
	const float defFov = 60.0f;
	const float defNear = 0.1f;
	const float defFar = 1000.0f;
	const Math::Vec3f scale{ 1,1,1 };
	const Math::Vec3f rotate{ 0,0,0 };
	const Math::Vec3f translate{ 0,0,-25 };

	auto fb = dx12Mgr_->GetFramebuffer();
	float width = static_cast<float>(fb->GetWidth());
	float height = static_cast<float>(fb->GetHeight());
	float aspect = (height > 0.0f) ? (width / height) : (1280.0f / 720.0f);

	out.fovY = Math::Func::NUM::ToRadians(defFov);
	out.aspectRatio = aspect;
	out.nearPlane = defNear;
	out.farPlane = defFar;
	out.position = translate;

	Math::Mat4x4 camWorld =
		Math::Func::MAT4x4::AffineMatrix(scale, rotate, translate);

	out.view = camWorld.Inverse();

	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		out.fovY,
		out.aspectRatio,
		out.nearPlane,
		out.farPlane
	);

	out.viewProj = out.view * out.proj;

	// forward（+Z）
	out.forward = Math::Vec3f{ 0,0,1 };

	out.valid = true;
}
