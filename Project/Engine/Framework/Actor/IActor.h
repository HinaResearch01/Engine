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
	virtual ~IActor() = default;

	// ===============================================
	// LifeCycle（Actor自身の責務のみ）
	// ===============================================
	virtual void Init() = 0;
	virtual void Finalize() {};

	// ===============================================
	// Component Management
	// ===============================================
	template<typename T, typename... Args>
	T* AddComp(Args&&... args) {
		static_assert(std::is_base_of_v<IComponent, T>);

		// Transformは必須＆1つ
		if constexpr (std::is_same_v<T, TransformComponent>) {
			return GetComponent<TransformComponent>();
		}

		const std::type_index key = typeid(T);
		if (comps_.contains(key)) {
			return static_cast<T*>(comps_[key].get());
		}

		auto comp = std::make_unique<T>(std::forward<Args>(args)...);
		comp->SetOwner(this);
		comp->Init();

		T* ptr = comp.get();
		comps_[key] = std::move(comp);

		NotifyComponentAdded(ptr);
		return ptr;
	}

	template<typename T>
	bool RemoveComp() {
		static_assert(std::is_base_of_v<IComponent, T>);

		if constexpr (std::is_same_v<T, TransformComponent>) {
			return false;
		}

		const std::type_index key = typeid(T);
		auto it = comps_.find(key);
		if (it == comps_.end()) return false;

		IComponent* removed = it->second.get();
		NotifyComponentRemoved(removed);

		comps_.erase(it);
		return true;
	}

	template<typename T>
	bool HasComp() const {
		if (comps_.contains(typeid(T))) return true;

		for (auto& [_, comp] : comps_) {
			if (dynamic_cast<T*>(comp.get())) return true;
		}
		return false;
	}

	template<typename T>
	T* GetComponent() {
		auto it = comps_.find(typeid(T));
		if (it != comps_.end()) return dynamic_cast<T*>(it->second.get());

		for (auto& [_, comp] : comps_) {
			if (auto* ptr = dynamic_cast<T*>(comp.get())) return ptr;
		}
		return nullptr;
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

	bool IsPaused() const { return state_ == State::Paused; }
#pragma endregion

private:
	void EnsureTransform();
	void NotifyComponentAdded(IComponent* c);
	void NotifyComponentRemoved(IComponent* c);

protected:
	State state_ = State::Active;
	ActorID id_ = 0;

	std::string name_;
	std::bitset<32> tags_{};

	std::unordered_map<std::type_index, std::unique_ptr<IComponent>> comps_;
	World* world_ = nullptr;
};

} 
