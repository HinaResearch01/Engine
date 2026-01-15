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

#include "ActorTag.h"
#include "Math/TMath.h"
#include "../Component/IComponent.h"
#include "../Component/Transform/TransformComponent.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* ゲーム内オブジェクトの基底クラス */
class IActor {

public:
	using ActorID = uint64_t;

	enum class State {
		None = -1,
		Active,
		Paused,
		Dead,
	};

public:
	explicit IActor();
	virtual ~IActor();

	// ===============================================
	// LifeCycle（Actor自身の責務のみ）
	// ===============================================
	virtual void Init() = 0;
	virtual void Finalize();

	// ===============================================
	// Component Management
	// ===============================================

	/// <summary>
	/// コンポーネント追加（型で1つまで）
	/// </summary>
	template<typename T, typename... Args>
	T* AddComp(Args&&... args) {
		static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

		std::lock_guard<std::mutex> lock(mutex_);

		// Transform は必須＆1つのみ
		if constexpr (std::is_same_v<T, TransformComponent>) {
			auto it = comps_.find(typeid(TransformComponent));
			return it != comps_.end()
				? static_cast<T*>(it->second.get())
				: nullptr;
		}

		// 既に存在する場合は差し替えない
		auto key = std::type_index(typeid(T));
		if (comps_.contains(key)) {
			return dynamic_cast<T*>(comps_[key].get());
		}

		auto comp = std::make_shared<T>(std::forward<Args>(args)...);
		comp->SetOwner(this);
		comp->Init();

		comps_[key] = comp;

		return comp.get();
	}

	/// <summary>
	/// コンポーネント削除
	/// </summary>
	template<typename T>
	bool RemoveComp() {
		static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

		if constexpr (std::is_same_v<T, TransformComponent>) {
			return false; // Transformは削除不可
		}

		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return false;

		comps_.erase(it);
		return true;
	}

	// ===============================================
	// Component Access
	// ===============================================

	template<typename T>
	bool HasComp() const {
		return comps_.contains(typeid(T));
	}

	template<typename T>
	T* GetComponent() {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return nullptr;
		return dynamic_cast<T*>(it->second.get());
	}

	template<typename T>
	std::shared_ptr<T> GetComponentShared() {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = comps_.find(typeid(T));
		if (it == comps_.end()) return nullptr;
		return std::dynamic_pointer_cast<T>(it->second);
	}

	template<typename F>
	void ForEachComponent(F&& func) {
		for (auto& [_, comp] : comps_) {
			func(comp.get());
		}
	}

	// ===============================================
	// Collision / Events
	// ===============================================
	virtual void OnCollision() {}

#pragma region Accessor
	ActorID GetID() const { return id_; }
	void SetID(ActorID id) { id_ = id; }

	State GetState() const { return state_; }
	void SetState(State s) { state_ = s; }

	World* GetWorld() const { return world_; }
	void SetWorld(World* w) { world_ = w; }
#pragma endregion

private:
	void EnsureTransform();

protected:
	State state_ = State::None;
	ActorID id_ = 0;

	std::bitset<32> tags_{};

	std::unordered_map<std::type_index, std::shared_ptr<IComponent>> comps_;

	World* world_ = nullptr;

	mutable std::mutex mutex_;
};

} 
