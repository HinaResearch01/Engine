#pragma once

#include <memory>
#include <vector>
#include <type_traits>

#include "CompView/ComponentView.h"
#include "Framework/Camera/CameraSystem.h"
#include "Framework/Render/RenderSystem.h"
#include "Framework/Actor/IActor.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class GameContext;

/* シーンの基底クラス */
class IScene {

public:
	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~IScene();

	// ===============================================
	// Scene LifeCycle
	// ===============================================
	virtual void Init() {}
	virtual void Update(float deltaTime) {
		UpdateActors(deltaTime);
		CleanupDeadActorsInternal();
	}
	virtual void Finalize() {
		ClearComponentView();
		actors_.clear();
	}

	// ===============================================
	// Actor Management
	// ===============================================

	/// Actor生成
	template<class T, class... Args>
	T* SpawnActor(Args&&... args) {
		static_assert(std::is_base_of_v<IActor, T>, "T must derive from IActor");

		auto actor = std::make_unique<T>(std::forward<Args>(args)...);
		actor->SetID(GenerateActorID());
		actor->Init();

		T* ptr = actor.get();
		actors_.push_back(std::move(actor));

		// View 登録
		RegisterComponentView(ptr);
		return ptr;
	}

	/// Actor一覧取得（System用）
	const std::vector<std::unique_ptr<IActor>>& GetActors() const {
		return actors_;
	}

	/// ActorID発行
	IActor::ActorID GenerateActorID() { return nextActorId_++; }

#pragma region Accessor
	// GameContext
	GameContext* GetGameContext() const { return gameContext_; }
	void SetGameContext(GameContext* context) { gameContext_ = context; }
	// CameraContext
	CameraContext& GetCameraContext() { return cameraContext_; }
	void SetCameraContext(const CameraContext& context) { cameraContext_ = context; }
	// ViewComp
	ComponentView<RenderComponent>& GetRenderables() { return renderables_; }
	ComponentView<CameraComponent>& GetCameras() { return cameras_; }
#pragma endregion

protected:
	// ===============================================
	// Internal Update
	// ===============================================

	void UpdateActors(float dt) {
		for (auto& a : actors_) {
			a->UpdateActor(dt);
		}
	}

	void CleanupDeadActorsInternal() {
		// View から先に除外
		for (auto& a : actors_) {
			if (a->GetState() == IActor::State::Dead) {
				UnregisterComponentViewActor(a.get());
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
	// シーン内のアクター群
	std::vector<std::unique_ptr<IActor>> actors_;
	// 次のアクターID
	IActor::ActorID nextActorId_ = 1;
	// 描画システム
	//RenderSystem renderSystem_;
	// コンポーネントビュー
	ComponentView<RenderComponent> renderables_;
	ComponentView<CameraComponent> cameras_;
	// ゲームコンテキスト
	GameContext* gameContext_ = nullptr;
	// カメラコンテキスト
	CameraContext cameraContext_;
};

}