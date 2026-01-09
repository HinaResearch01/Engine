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
#include "Framework/System/RenderSystem.h"
#include "Framework/Scene/CompView/ComponentView.h"

namespace Tsumi::Framework {

// 前方宣言
class GameContext;

/* シーン（World）の基底クラス */
class World {

public:
	World()
		: transformSystem_(*this), cameraSystem_(*this), renderSystem_(*this)
	{
		//  System系をUpdateManagerに登録
		updateMgr_.Register(&transformSystem_);
		updateMgr_.Register(&cameraSystem_);
		updateMgr_.Register(&renderSystem_);
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
	// System Accessor
	// ===============================================

	template<class T>
	T* GetSystem() {
		if constexpr (std::is_same_v<T, TransformSystem>) {
			return &transformSystem_;
		}
		else if constexpr (std::is_same_v<T, CameraSystem>) {
			return &cameraSystem_;
		}
		else if constexpr (std::is_same_v<T, RenderSystem>) {
			return &renderSystem_;
		}
		else {
			return nullptr;
		}
	}

	// ===============================================
	// Actor Management
	// ===============================================

	template<class T, class... Args>
	T* SpawnActor(Args&&... args) {
		static_assert(std::is_base_of_v<IActor, T>, "T must derive from IActor");

		auto actor = std::make_unique<T>(std::forward<Args>(args)...);
		actor->SetID(GenerateActorID()); // IDの設定
		actor->SetWorld(this);			 // Worldを知らせる
		actor->Init();					 // 初期化処理
		T* ptr = actor.get();
		actors_.push_back(std::move(actor));

		// ComponentViewに登録
		RegisterActorToComponentViews(ptr);
		 
		return ptr;
	}

	const std::vector<std::unique_ptr<IActor>>& GetActors() const {
		return actors_;
	}

	IActor::ActorID GenerateActorID() { return nextActorId_++; }

#pragma region Accessor
	GameContext* GetGameContext() const { return gameContext_; }
	void SetGameContext(GameContext* context) { gameContext_ = context; }

	ComponentView<TransformComponent>& GetTransformsCompView() { return transformsView_; }
	ComponentView<CameraComponent>& GetCamerasCompView() { return camerasView_; }
	ComponentView<RenderComponent>& GetRenderCompView() { return rendersView_; }

	UpdateManager& GetUpdateManager() { return updateMgr_; }
#pragma endregion

protected:
	// ===============================================
	// UpdateManager Management
	// ===============================================

	void RegisterActorUpdatables(IActor* actor) {
		actor->ForEachComponent([this](IComponent* comp) {
			if (auto* updatable = dynamic_cast<IUpdatable*>(comp)) {
				updateMgr_.Register(updatable);
			}
		});
	}

	void UnregisterActorUpdatables(IActor* actor) {
		actor->ForEachComponent([this](IComponent* comp) {
			if (auto* updatable = dynamic_cast<IUpdatable*>(comp)) {
				updateMgr_.UnRegister(updatable);
			}
		});
	}

	// ===============================================
	// ComponentView Management
	// ===============================================

	void RegisterActorToComponentViews(IActor* actor) {
		if (!actor) return;

		transformsView_.Refresh(actor);
		camerasView_.Refresh(actor);
		rendersView_.Refresh(actor);

		// IUpdatable をまとめて登録
		RegisterActorUpdatables(actor);
	}

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

				UnregisterActorUpdatables(a.get());

				// Component / Update の解除は Actor 側の責務
				a->Finalize();
			}
		}

		std::erase_if(actors_, [](const std::unique_ptr<IActor>& a) {
			return a->GetState() == IActor::State::Dead;
		});
	}

protected:
	// ===== Actor 管理 =====
	std::vector<std::unique_ptr<IActor>> actors_;
	IActor::ActorID nextActorId_ = 1;

	// ===== 各種システム =====
	UpdateManager updateMgr_;
	TransformSystem transformSystem_;
	CameraSystem cameraSystem_;
	RenderSystem renderSystem_;

	// ===== ComponentView =====
	ComponentView<TransformComponent> transformsView_;
	ComponentView<CameraComponent> camerasView_;
	ComponentView<RenderComponent> rendersView_;

	// ===== GameContext =====
	GameContext* gameContext_ = nullptr;
};

} 