#pragma once

#include <memory>
#include <vector>
#include <type_traits>
#include <algorithm>

#include "Framework/Actor/IActor.h"
#include "Framework/Update/UpdateManager.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Component/IComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Scene/CompView/ComponentView.h"

namespace Tsumi::Framework {

// 前方宣言
class GameContext;

/* シーン（World）の基底クラス */
class World {

public:
	virtual ~World() = default;

	// ===============================================
	// World LifeCycle
	// ===============================================
	virtual void Init() {}

	virtual void Update(float deltaTime) {
		// 更新の主体は UpdateManager
		updateMgr_.ExecuteUpdate(deltaTime);

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

		// ★ World を Actor に知らせる（重要）
		actor->SetWorld(this);

		actor->Init();

		T* ptr = actor.get();
		actors_.push_back(std::move(actor));

		// Actor単位で View 登録
		RegisterComponentView(ptr);
		return ptr;
	}

	const std::vector<std::unique_ptr<IActor>>& GetActors() const {
		return actors_;
	}

	IActor::ActorID GenerateActorID() { return nextActorId_++; }

#pragma region Accessor
	GameContext* GetGameContext() const { return gameContext_; }
	void SetGameContext(GameContext* context) { gameContext_ = context; }

	ComponentView<RenderComponent>& GetRenderables() { return renderables_; }
	ComponentView<CameraComponent>& GetCameras() { return cameras_; }

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
		renderables_.Refresh(actor);
		cameras_.Refresh(actor);

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

		renderables_.Refresh(actor);
		cameras_.Refresh(actor);
	}

protected:
	// ===============================================
	// Dead Actor Cleanup
	// ===============================================

	void CleanupDeadActorsInternal() {
		for (auto& a : actors_) {
			if (a->GetState() == IActor::State::Dead) {
				// View から先に除外
				UnregisterComponentViewActor(a.get());

				// Component 側で OnComponentRemoved が呼ばれる前提
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

	void RegisterComponentView(IActor* actor) {
		renderables_.Refresh(actor);
		cameras_.Refresh(actor);
	}

	void UnregisterComponentViewActor(IActor* actor) {
		renderables_.Remove(actor);
		cameras_.Remove(actor);
	}

	void ClearComponentView() {
		renderables_.Clear();
		cameras_.Clear();
	}

protected:
	// Actor 群
	std::vector<std::unique_ptr<IActor>> actors_;

	// ActorID
	IActor::ActorID nextActorId_ = 1;

	// Update の中核
	UpdateManager updateMgr_;

	// Component Views（索引）
	ComponentView<RenderComponent> renderables_;
	ComponentView<CameraComponent> cameras_;

	// GameContext
	GameContext* gameContext_ = nullptr;
};

} 