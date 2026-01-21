#pragma once

#include <memory>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <unordered_map>
#include <typeindex>
#include <cassert>

#include "Framework/World/CompView/ComponentView.h"
#include "Framework/World/CompView/ComponentViewRange.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Component/IComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Update/UpdateManager.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/System/Light/LightSystem.h"
#include "Framework/System/Material/MaterialSystem.h"
#include "Framework/System/Render/RenderSystem.h"
#include "Framework/System/Transform/TransformSystem.h"

namespace Tsumi::Framework {

// 前方宣言
class GameContext;

/* シーン（World）の基底クラス */
class World {

public:
	World()
		: cameraSystem_(*this), lightSystem_(*this), materialSystem_(*this), renderSystem_(*this), transformSystem_(*this)
	{
		//  System系をUpdateManagerに登録
		updateMgr_.Register(&cameraSystem_);
		updateMgr_.Register(&materialSystem_);
		updateMgr_.Register(&renderSystem_);
		updateMgr_.Register(&transformSystem_);

		// Viewの登録
		RegisterDefaultViews();
	}
	virtual ~World() = default;

	// ===============================================
	// World LifeCycle
	// ===============================================
	virtual void Init() {}
	virtual void Update(float deltaTime) {
		WorldUpdate(deltaTime);
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
		if constexpr (std::is_same_v<T, CameraSystem>) {
			return &cameraSystem_;
		}
		else if constexpr (std::is_same_v<T, MaterialSystem>) {
			return &materialSystem_;
		}
		else if constexpr (std::is_same_v<T, RenderSystem>) {
			return &renderSystem_;
		}
		else if constexpr (std::is_same_v<T, TransformSystem>) {
			return &transformSystem_;
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

	// ===============================================
	// ComponentView View 
	// ===============================================

	template<class T>
	ComponentView<T>& GetView()
	{
		const std::type_index ti = typeid(T);
		auto it = views_.find(ti);
		assert(it != views_.end());
		return *static_cast<ComponentView<T>*>(it->second);
	}

	template<class... Cs>
	auto View()
	{
		const std::vector<IActor*>* base = nullptr;
		size_t minSize = SIZE_MAX;

		auto pickBase = [&](const auto& view)
		{
			const auto& v = view.GetActors();
			if (v.size() < minSize)
			{
				minSize = v.size();
				base = &v;
			}
		};

		(pickBase(GetView<Cs>()), ...);

		assert(base);
		return ViewRange<Cs...>(*this, *base);
	}

#pragma region Accessor
	GameContext* GetGameContext() const { return gameContext_; }
	void SetGameContext(GameContext* context) { gameContext_ = context; }

	ComponentView<CameraComponent>& GetCamerasCompView() { return cameraCompView_; }
	ComponentView<MaterialComponent>& GetMaterialsCompView() { return materialCompView_; }
	ComponentView<RenderComponent>& GetRenderCompView() { return renderCompView_; }
	ComponentView<TransformComponent>& GetTransformsCompView() { return transformCompView_; }

	UpdateManager& GetUpdateManager() { return updateMgr_; }
#pragma endregion

protected:
	// ===============================================
	// World Update
	// ===============================================
	void WorldUpdate(float deltaTime) {
		// 更新の主体は UpdateManager
		updateMgr_.Execute(deltaTime);
		// Dead Actor の後処理
		CleanupDeadActorsInternal();
	}

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

	template<class T>
	void RegisterView(ComponentView<T>& view)
	{
		views_[typeid(T)] = &view;
	}

	void RegisterDefaultViews()
	{
		// ここで World::View<T>() が生きる
		RegisterView(cameraCompView_);
		RegisterView(materialCompView_);
		RegisterView(renderCompView_);
		RegisterView(transformCompView_);
	}

	void RegisterActorToComponentViews(IActor* actor) {
		if (!actor) return;

		// 全Viewを走査してRefresh
		for (auto& [type, viewBase] : views_)
			viewBase->Refresh(actor);

		RegisterActorUpdatables(actor);
	}

	void UnregisterComponentViewActor(IActor* actor) {
		if (!actor) return;

		for (auto& [type, viewBase] : views_)
			viewBase->Remove(actor);
	}

	void ClearComponentView() {
		for (auto& [type, viewBase] : views_)
			viewBase->Clear();
	}

	// ===============================================
	// Dead Actor Cleanup
	// ===============================================

	void CleanupDeadActorsInternal() {
		for (auto& a : actors_) {
			if (a->GetState() == IActor::State::Dead) {

				// View から除外（Actor単位）
				UnregisterComponentViewActor(a.get());
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
	CameraSystem cameraSystem_;
	LightSystem lightSystem_;
	MaterialSystem materialSystem_;
	RenderSystem renderSystem_;
	TransformSystem transformSystem_;

	// ===== ComponentView =====
	std::unordered_map<std::type_index, IComponentViewBase*> views_;
	ComponentView<CameraComponent> cameraCompView_;
	ComponentView<MaterialComponent> materialCompView_;
	ComponentView<RenderComponent> renderCompView_;
	ComponentView<TransformComponent> transformCompView_;

	// ===== GameContext =====
	GameContext* gameContext_ = nullptr;
};

}