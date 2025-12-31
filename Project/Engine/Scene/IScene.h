#pragma once

#include <memory>
#include <vector>
#include <type_traits>

#include "Framework/Actor/IActor.h"

namespace Tsumi {

// 前方宣言
class GameContext;

/* シーンの基底クラス */
class IScene {

public:
	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~IScene() = default;


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
		static_assert(std::is_base_of_v<Framework::IActor, T>, "T must derive from IActor");

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
					  [](const std::unique_ptr<Framework::IActor>& actor) {
			return actor->GetState() == Framework::IActor::State::Dead;
		});
	}

	/// <summary>
	/// Actor一覧取得（System用）
	/// </summary>
	const std::vector<std::unique_ptr<Framework::IActor>>& GetActors() const {
		return actors_;
	}

#pragma region Accessor
	void SetContext(GameContext* context) { gameContext_ = context; }
	GameContext* GetContext() const { return gameContext_; }
#pragma endregion

protected:
	GameContext* gameContext_ = nullptr;

	// シーン内のアクター群
	std::vector<std::unique_ptr<Framework::IActor>> actors_;
};

}