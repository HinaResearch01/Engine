#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <type_traits>
#include <mutex>
#include "Math/TMath.h"
#include "../Component/IComponent.h"

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
	virtual ~IActor(){ comps_.clear(); }

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
		for (auto& component : comps_) {
			component.second->Update();
		}
		if (isRender_) {
			rendComp_->Update();
		}
	}

	/// <summary>
	/// 描画処理
	/// </summary>
	void Render() {
		if (isRender_) {
			rendComp_->Render();
		}
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
		std::lock_guard<std::mutex> lock(mutex_);
		// 型情報でIComponentを保存 (同じコンポーネントは1つまで)
		comps_[typeid(T)] = comp;
	}
	template<typename T, typename... Args>
	void AddRendComp(Args&&... args) {
		if(rendComp_) return; // 既にrendCompがあれば早期return
		// TがIComponentクラスを継承していなければエラー
		static_assert(std::is_base_of<IComponent, T>::value, "T must derive from Component");
		std::shared_ptr comp = std::make_shared<T>(std::forward<Args>(args)...);
		comp->SetOwner(this);
		std::lock_guard<std::mutex> lock(mutex_);
		// 型情報でIComponentを保存 (同じコンポーネントは1つまで)
		rendComp_= comp;
		isRender_ = true;
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

#pragma endregion 

private:
	// 名前
	std::string name_ = "default";

	// 状態
	State state_ = IActor::State::None;

	// 通常コンポーネント
	std::unordered_map<std::type_index, std::shared_ptr<IComponent>> comps_;
	// 描画用コンポーネント (ActorがもてるRenderCompは1つまで)
	std::shared_ptr<IComponent> rendComp_;
	bool isRender_ = false; // 描画フラグ

	std::mutex mutex_;
};
}
