#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <type_traits>
#include <mutex>
#include <optional>
#include <bitset>

#include "Math/TMath.h"
#include "ActorTag.h"
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
	IActor();
	IActor(const std::string& name);

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~IActor() {
		std::lock_guard<std::mutex> lock(mutex_);
		comps_.clear();
		transComp_.reset();
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
		if (state_ != State::Active) return;
		Update(deltaTime);
		UpdateComponents(deltaTime);
	}

	/// <summary>
	/// コンポーネントの追加（型で1つまで）
	/// </summary>
	template<typename T, typename... Args>
	T* AddComp(Args&&... args) {
		static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

		// Transform は必須＆特別扱い（必ず保持したい）
		if constexpr (std::is_same_v<T, TransformComponent>) {
			std::lock_guard<std::mutex> lock(mutex_);
			if (!transComp_) {
				transComp_ = std::make_shared<TransformComponent>(std::forward<Args>(args)...);
				transComp_->SetOwner(this);
				transComp_->Init();
				comps_[typeid(TransformComponent)] = transComp_;
			}
			return transComp_.get();
		}

		auto comp = std::make_shared<T>(std::forward<Args>(args)...);
		comp->SetOwner(this);
		comp->Init();

		std::lock_guard<std::mutex> lock(mutex_);
		comps_[typeid(T)] = comp;
		return comp.get();
	}

	/// <summary>
	/// コンポーネント削除（型で指定）
	/// </summary>
	template<typename T>
	bool RemoveComp() {
		// Transformは必須なので削除不可
		if constexpr (std::is_same_v<T, TransformComponent>) {
			return false;
		}

		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return false;

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
	/// 指定型のコンポーネント取得（生ポインタ）
	/// </summary>
	template<typename T>
	T* GetComponent() {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return nullptr;
		return dynamic_cast<T*>(it->second.get());
	}

	/// <summary>
	/// 指定型のコンポーネント取得（shared_ptr）
	/// </summary>
	template<typename T>
	std::shared_ptr<T> GetComponentShared() {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return nullptr;
		return std::dynamic_pointer_cast<T>(it->second);
	}

	// トランスフォームコンポーネント
	TransformComponent* GetTransform() {
		std::lock_guard<std::mutex> lock(mutex_);
		return transComp_.get();
	}

	/// <summary>
	/// 衝突時コールバック関数
	/// </summary>
	virtual void OnCollision() {};

#pragma region Accessor
	// 名前
	std::string GetName() const { return name_; }
	// 状態
	State GetState() const { return state_; }
	void SetState(State state) { state_ = state; }
#pragma endregion 

protected:
	/// <summary>
	/// コンポーネント更新
	/// </summary>
	void UpdateComponents([[maybe_unused]] float deltaTime) {
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& kv : comps_) {
			if (kv.second) kv.second->Update();
		}
	}

private:
	/// <summary>
	/// TransformComponent の確保
	/// </summary>
	void EnsureTransform() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (transComp_) return;

		transComp_ = std::make_shared<TransformComponent>();
		transComp_->SetOwner(this);
		transComp_->Init();

		comps_[typeid(TransformComponent)] = transComp_;
	}

private:
	// 名前
	std::string name_ = "default";

	// 状態
	State state_ = State::None;

	// タグ（弾・地形など分類用）
	std::bitset<32> tags_{};

	// コンポーネント（型で1つまで）
	std::unordered_map<std::type_index, std::shared_ptr<IComponent>> comps_;

	// Transform（必須）
	std::shared_ptr<TransformComponent> transComp_;

	// mutex for thread-safety
	mutable std::mutex mutex_;
};
}
