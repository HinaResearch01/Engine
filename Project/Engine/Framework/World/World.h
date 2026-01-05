#pragma once

#include <memory>
#include <vector>
#include <type_traits>

#include "Framework/Actor/IActor.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Scene/CompView/ComponentView.h"
#include "Framework/View/CameraContext.h"

namespace Tsumi::Framework {

// 前方宣言
class GameContext;

/* シーンの基底クラス */
class World {

public:
	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~World() {};

	// ===============================================
	// Scene LifeCycle
	// ===============================================
	virtual void Init() {}
	virtual void Update(float deltaTime) {
		deltaTime; // 未使用回避
	}
	virtual void Finalize() {
		ClearComponentView();
		actors_.clear();
	}

	// ===============================================
	// Actor Management
	// ===============================================

	/// <summary>
	/// Actor生成
	/// </summary>
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

	/// <summary>
	/// Actor一覧取得
	/// </summary>
	const std::vector<std::unique_ptr<IActor>>& GetActors() const {
		return actors_;
	}

	/// <summary>
	/// ActorID発行
	/// </summary>
	IActor::ActorID GenerateActorID() { return nextActorId_++; }

#pragma region Accessor
	// GameContext
	GameContext* GetGameContext() const { return gameContext_; }
	void SetGameContext(GameContext* context) { gameContext_ = context; }
	// ViewComp
	ComponentView<RenderComponent>& GetRenderables() { return renderables_; }
	ComponentView<CameraComponent>& GetCameras() { return cameras_; }
#pragma endregion

protected:
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
	// コンポーネントビュー
	ComponentView<RenderComponent> renderables_;
	ComponentView<CameraComponent> cameras_;
	// ゲームコンテキスト
	GameContext* gameContext_ = nullptr;
};

}