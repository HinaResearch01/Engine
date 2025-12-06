#pragma once

#include "IScene.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace Tsumi {

/* シーン管理クラス */
class SceneManager {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	SceneManager() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~SceneManager() = default;

    /// <summary>
    /// シーンの登録
    /// </summary>
    template<typename T, typename... Args>
    void RegisterScene(const std::string& name, Args&&... args);

    /// <summary>
    /// シーンの変更
    /// </summary>
    void ChangeScene(const std::string& name);

	/// <summary>
	/// 現在のsceneの初期化処理
	/// </summary>
	void Init();

    /// <summary>
    /// 現在のsceneの更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 現在のsceneの描画処理
    /// </summary>
    void RenderBackSprite();
    void RendModeler();
    void RenderFrontSprite();

private:
	std::unordered_map<std::string, std::unique_ptr<IScene>> scenes_;
	IScene* currentScene_ = nullptr;
	bool pendingInit_ = false;
};


template<typename T, typename ...Args>
inline void SceneManager::RegisterScene(const std::string& name, Args && ...args)
{
	static_assert(std::is_base_of<IScene, T>::value,
		"RegisterScene: T must derive from IScene");

	using SceneType = T;
	using BaseType = IScene;

	std::unique_ptr<BaseType> ptr(
		new SceneType(std::forward<Args>(args)...)
	);

	scenes_[name] = std::move(ptr);
}

inline void SceneManager::ChangeScene(const std::string& name)
{
	if (currentScene_)
		currentScene_->Finalize();

	currentScene_ = scenes_[name].get();
	currentScene_->SetManager(this);

	pendingInit_ = true;
}

inline void SceneManager::Init()
{
	if (pendingInit_) 
		currentScene_->Init(), pendingInit_ = false;
}

inline void SceneManager::Update()
{
	if (currentScene_) currentScene_->Update();
}

inline void SceneManager::RenderBackSprite()
{
	if (currentScene_) currentScene_->RenderBackSprite();
}

inline void SceneManager::RendModeler()
{
	if (currentScene_) currentScene_->RenderModel();
}

inline void SceneManager::RenderFrontSprite()
{
	if (currentScene_) currentScene_->RenderFrontSprite();
}

}