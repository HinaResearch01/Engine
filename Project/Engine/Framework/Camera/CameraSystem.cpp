#include "CameraSystem.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Scene/IScene.h"
#include "Utils/Logger/Logger.h"

using namespace Tsumi::Framework;

void CameraSystem::Update(IScene& scene)
{
	CameraContext& camCtx = scene.GetCameraContext();
	camCtx.valid = false;

	IActor* camActor = nullptr;
	camActor;

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
	actor, out;
}
