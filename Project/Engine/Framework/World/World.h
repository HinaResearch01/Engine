#pragma once

#include <memory>
#include <vector>
#include <type_traits>
#include <algorithm>

#include "Framework/Actor/IActor.h"
#include "Framework/Component/IComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Update/UpdateManager.h"
#include "Framework/System/TransformSystem.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/Scene/CompView/ComponentView.h"

namespace Tsumi::Framework {

// 前方宣言
class GameContext;

/* シーン（World）の基底クラス */
class World {

public:
	World()
		: transformSystem_(*this), cameraSystem_(*this)
	{
		//  System系をUpdateManagerに登録
		updateMgr_.Register(&transformSystem_);
		updateMgr_.Register(&cameraSystem_);
	}
	virtual ~World() = default;

	// ===============================================
	// World LifeCycle
	// ===============================================
	virtual void Init() {}

	virtual void Update(float deltaTime) {
		// 更新の主体は UpdateManager
		updateMgr_.Execute(deltaTime);

		// Dead Actor の後処理
		CleanupDeadActorsInternal();
	}

	virtual void Finalize() {
		ClearComponentView();
		actors_.clear();
	}

	// ===============================================
	// Actor Management
	// ===============================================

	template<class T, class... Args>
	T* SpawnActor(Args&&... args) {
		static_assert(std::is_base_of_v<IActor, T>, "T must derive from IActor");

		auto actor = std::make_unique<T>(std::forward<Args>(args)...);
		actor->SetID(GenerateActorID());

		// World を Actor に知らせる
		actor->SetWorld(this);

		actor->Init();

		T* ptr = actor.get();
		actors_.push_back(std::move(actor));
		 
		return ptr;
	}

	const std::vector<std::unique_ptr<IActor>>& GetActors() const {
		return actors_;
	}

	IActor::ActorID GenerateActorID() { return nextActorId_++; }

#pragma region Accessor
	GameContext* GetGameContext() const { return gameContext_; }
	void SetGameContext(GameContext* context) { gameContext_ = context; }

	ComponentView<TransformComponent>& GetTransforms() { return transformsView_; }
	ComponentView<CameraComponent>& GetCameras() { return camerasView_; }
	ComponentView<RenderComponent>& GetRenderables() { return rendersView_; }

	UpdateManager& GetUpdateManager() { return updateMgr_; }
#pragma endregion

	// ===============================================
	// Component Registration Hook（最重要）
	// ===============================================

	/// <summary>
	/// Actor が Component を追加したときに呼ぶ
	/// </summary>
	void OnComponentAdded(IActor* actor, IComponent* comp) {
		if (!actor || !comp) return;

		// View 更新（索引）
		transformsView_.Refresh(actor);
		camerasView_.Refresh(actor);
		rendersView_.Refresh(actor);

		// IUpdatable なら自動登録
		if (auto* updatable = dynamic_cast<IUpdatable*>(comp)) {
			updateMgr_.Register(updatable);
		}
	}

	/// <summary>
	/// Actor が Component を削除したときに呼ぶ
	/// </summary>
	void OnComponentRemoved(IActor* actor, IComponent* comp) {
		if (!actor || !comp) return;

		if (auto* updatable = dynamic_cast<IUpdatable*>(comp)) {
			updateMgr_.UnRegister(updatable);
		}

		transformsView_.Refresh(actor);
		camerasView_.Refresh(actor);
		rendersView_.Refresh(actor);
	}

protected:
	// ===============================================
	// Dead Actor Cleanup
	// ===============================================

	void CleanupDeadActorsInternal() {
		for (auto& a : actors_) {
			if (a->GetState() == IActor::State::Dead) {

				// View から除外（Actor単位）
				transformsView_.Remove(a.get());
				camerasView_.Remove(a.get());
				rendersView_.Remove(a.get());

				// Component / Update の解除は Actor 側の責務
				a->Finalize();
			}
		}

		std::erase_if(actors_, [](const std::unique_ptr<IActor>& a) {
			return a->GetState() == IActor::State::Dead;
		});
	}

	// ===============================================
	// ComponentView Management
	// ===============================================

	void UnregisterComponentViewActor(IActor* actor) {
		transformsView_.Remove(actor);
		camerasView_.Remove(actor);
		rendersView_.Remove(actor);
	}

	void ClearComponentView() {
		transformsView_.Clear();
		camerasView_.Clear();
		rendersView_.Clear();
	}

protected:
	// ===== Actor 管理 =====
	std::vector<std::unique_ptr<IActor>> actors_;
	IActor::ActorID nextActorId_ = 1;

	// ===== 各種システム =====
	UpdateManager updateMgr_;
	TransformSystem transformSystem_;
	CameraSystem cameraSystem_;

	// ===== ComponentView =====
	ComponentView<TransformComponent> transformsView_;
	ComponentView<CameraComponent> camerasView_;
	ComponentView<RenderComponent> rendersView_;

	// ===== GameContext =====
	GameContext* gameContext_ = nullptr;
};

} 