#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "Framework/Scene/IScene.h"

namespace Tsumi::Framework {

class GameContext {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	GameContext();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~GameContext() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 各描画処理
	/// </summary>
	void BKSpriteRender();
	void ModelRender();
	void FTSpriteRender();

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// シーンの登録
	/// </summary>
	template<typename T, typename... Args>
	void RegisterScene(const std::string&, Args&&... args);

	/// <summary>
	/// シーンの変更
	/// </summary>
	void ChangeScene(const std::string& name);


#pragma region Accessor
	bool GetPedingInit() const { return pendingInit_; }
#pragma endregion

private:
	std::unordered_map<std::string, std::unique_ptr<Framework::IScene>> scenes_;
	Framework::IScene* currentScene_ = nullptr;
	bool pendingInit_ = false;
};


inline void GameContext::Init() { 
	if (!pendingInit_ || !currentScene_) return;
	currentScene_->Init(), pendingInit_ = false;
}
inline void GameContext::Update() {	if(currentScene_) currentScene_->Update(0); }
inline void GameContext::BKSpriteRender() { }
inline void GameContext::ModelRender() { }
inline void GameContext::FTSpriteRender() { }
inline void GameContext::Finalize() { if(currentScene_) currentScene_->Finalize(); }

template<typename T, typename ...Args>
inline void GameContext::RegisterScene(const std::string& name, Args && ...args)
{
	// TはISceneを継承していなければいけない
	static_assert(std::is_base_of<Framework::IScene, T>::value,
		"RegisterScene: T must derive from IScene");

	using SceneType = T;
	using BaseType = Framework::IScene;

	// mapに追加
	std::unique_ptr<BaseType> ptr(
		new SceneType(std::forward<Args>(args)...)
	);
	ptr->SetContext(this); // このクラスのptrを渡す
	scenes_[name] = std::move(ptr);
}

inline void GameContext::ChangeScene(const std::string& name) 
{
	auto it = scenes_.find(name);
	if (it == scenes_.end()) {
		return;
	}
	currentScene_ = it->second.get();
	pendingInit_ = true;
}

}