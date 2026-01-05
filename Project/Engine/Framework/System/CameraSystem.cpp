#include "CameraSystem.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Scene/IScene.h"
#include "Utils/Logger/Logger.h"

using namespace Tsumi::Framework;

void CameraSystem::Update(IScene& scene)
{
	// カメラコンテキスト取得
	CameraContext& camCtx = scene.GetCameraContext();
	camCtx.valid = false;

	// メインカメラ選択
	IActor* camActor = SelectMainCamera(scene.GetCameras());
	if (!camActor) return;

	// 行列構築
	BuildMatrices(camActor, camCtx);
	camCtx.valid = true;
}

IActor* CameraSystem::SelectMainCamera(const ComponentView<CameraComponent>& cameras)
{
	IActor* best = nullptr;
	int bestPriority = std::numeric_limits<int>::min();
	uint64_t bestId = UINT64_MAX;

	int mainCount = 0;

	for (auto* actor : cameras.Get()) {
		auto* cam = actor->GetComponent<CameraComponent>();
		if (!cam || !cam->enabled) continue;
		if (cam->role != CameraComponent::Role::Main) continue;

		++mainCount;

		const int p = cam->priority;
		const uint64_t id = actor->GetID();

		if (p > bestPriority ||
			(p == bestPriority && id < bestId)) {
			best = actor;
			bestPriority = p;
			bestId = id;
		}
	}

	// フォールバック：Main が無い場合
	if (!best) {
		for (auto* actor : cameras.Get()) {
			auto* cam = actor->GetComponent<CameraComponent>();
			if (cam && cam->enabled) {
				best = actor;
				break;
			}
		}
	}

	return best;
}

void CameraSystem::BuildMatrices(IActor* actor, CameraContext& out)
{
	// Component取得
	auto* trans = actor->GetTransform();
	auto* cam = actor->GetComponent<CameraComponent>();
	// 空チェック
	if (!trans || !cam) {
		return;
	}

	// ビュー行列
	out.view = trans->GetWorldMatrix().Inverse();
	// プロジェクション行列
	out.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(
		Math::Func::NUM::ToRadians(cam->fovY),
		cam->aspectRatio,
		cam->nearZ,
		cam->farZ);
	// ビュー・プロジェクション行列
	out.viewProj = out.view * out.proj;
	// カメラ位置
	out.position = trans->GetWorldPos();
}
