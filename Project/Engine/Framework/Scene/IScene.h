#pragma once

#include <memory>
#include <vector>
#include <type_traits>

#include "CompView/ComponentView.h"
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
		CleanupDeadActors();
	}
	virtual void Finalize() {
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
		actor->Init();

		T* ptr = actor.get();
		actors_.push_back(std::move(actor));
		return ptr;
	}

	/// <summary>
	/// 全Actor更新
	/// </summary>
	void UpdateActors(float deltaTime) {
		for (auto& actor : actors_) {
			actor->UpdateActor(deltaTime);
		}
	}

	/// <summary>
	/// Dead状態のActorを削除
	/// </summary>
	void CleanupDeadActors() {
		std::erase_if(actors_,
					  [](const std::unique_ptr<IActor>& actor) {
			return actor->GetState() == IActor::State::Dead;
		});
	}

	/// <summary>
	/// Actor一覧取得（System用）
	/// </summary>
	const std::vector<std::unique_ptr<IActor>>& GetActors() const {
		return actors_;
	}

#pragma region Accessor
	void SetContext(GameContext* context) { gameContext_ = context; }
	GameContext* GetContext() const { return gameContext_; }
#pragma endregion

protected:
	// シーン内のアクター群
	std::vector<std::unique_ptr<IActor>> actors_;
	// 描画システム
	//RenderSystem renderSystem_;
	// コンポーネントビュー
	ComponentView<RenderComponent> renderables_;
	ComponentView<CameraComponent> cameras_;
	// ゲームコンテキスト
	GameContext* gameContext_ = nullptr;
};

}