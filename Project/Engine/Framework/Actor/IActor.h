#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <type_traits>
#include <mutex>
#include <optional>
#include "Math/TMath.h"
#include "../Component/IComponent.h"
#include "../Component/Transform/TransformComponent.h"

namespace Tsumi::Framework {

/* ゲーム内オブジェクトの基底クラス */
class IActor {

public:
	// 状態
	enum class State {
		None = -1,
		Active,
		Paused,
		Dead,
	};

	/// <summary>
	/// コンストラクタ
	/// </summary>
	IActor() = default;
	IActor(const std::string& name);

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~IActor() {
		// safety: lock while clearing
		std::lock_guard<std::mutex> lock(mutex_);
		comps_.clear();
		transComp_.reset();
		renderType_.reset();
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Init() = 0;

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update([[maybe_unused]] float deltaTime) {};
	void UpdateActor([[maybe_unused]] float deltaTime) {
		// ステートアクティブ時、更新
		if (state_ == State::Active) {
			Update(deltaTime);
			UpdateComponents(deltaTime);
		}
	}
	void UpdateComponents([[maybe_unused]] float deltaTime) {
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& component : comps_) {
			if (component.second) component.second->Update();
		}
	}

	/// <summary>
	/// 描画処理
	/// </summary>
	void Render() {
		std::shared_ptr<IComponent> rend = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (renderType_) {
				auto it = comps_.find(*renderType_);
				if (it != comps_.end()) rend = it->second;
			}
		}
		if (rend) rend->Render();
	}

	/// <summary>
	/// コンポーネントの追加
	/// </summary>
	template<typename T, typename... Args>
	void AddComp(Args&&... args) {
		// TがIComponentクラスを継承していなければエラー
		static_assert(std::is_base_of<IComponent, T>::value, "T must derive from Component");
		auto comp = std::make_shared<T>(std::forward<Args>(args)...);
		comp->SetOwner(this);
		comp->Init();
		std::lock_guard<std::mutex> lock(mutex_);
		// 型情報でIComponentを保存 (同じコンポーネントは1つまで)
		comps_[typeid(T)] = comp;
	}

	/// <summary>
	/// 描画用コンポーネントを追加
	/// - Actor が持てる RenderComp は 1 つまで
	/// </summary>
	template<typename T, typename... Args>
	void AddRendComp(Args&&... args) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (renderType_) return; // 既に render comp が設定されていれば早期return
		static_assert(std::is_base_of<IComponent, T>::value, "T must derive from Component");
		auto comp = std::make_shared<T>(std::forward<Args>(args)...);
		comp->SetOwner(this);
		comp->Init();
		comps_[typeid(T)] = comp;
		renderType_ = typeid(T);
	}

	/// <summary>
	/// コンポーネント削除（型で指定）
	/// </summary>
	template<typename T>
	bool RemoveComp() {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return false;
		// if this was the render component, clear renderType_
		if (renderType_ && *renderType_ == typeid(T)) renderType_.reset();
		comps_.erase(it);
		return true;
	}

	/// <summary>
	/// 指定型のコンポーネントが存在するか
	/// </summary>
	template<typename T>
	bool HasComp() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return comps_.find(typeid(T)) != comps_.end();
	}

	/// <summary>
	/// 衝突時コールバック関数
	/// </summary>
	virtual void OnCollision() {};

#pragma region Accessor
	// 状態
	State GetState() const { return state_; }
	// コンポーネント
	template<typename T>
	T* GetComponent() {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return nullptr;
		return dynamic_cast<T*>(it->second.get());
	}
	// コンポーネント
	template<typename T>
	std::shared_ptr<T> GetComponentShared() {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return nullptr;
		return std::dynamic_pointer_cast<T>(it->second);
	}
	// トランスフォームコンポーネント
	std::weak_ptr<TransformComponent> GetTransComp() { return transComp_; }
#pragma endregion 

private:
	// 名前
	std::string name_ = "default";

	// 状態
	State state_ = IActor::State::None;

	// コンポーネント
	std::unordered_map<std::type_index, std::shared_ptr<IComponent>> comps_;

	// トランスフォームコンポーネント (Actorが必ず1つもつ)
	std::shared_ptr<TransformComponent> transComp_;

	// レンダーコンポーネントの型
	std::optional<std::type_index> renderType_;

	// mutex for thread-safety
	mutable std::mutex mutex_;
};
}
