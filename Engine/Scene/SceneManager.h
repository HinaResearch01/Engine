#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "IScene.h"

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
    void RegisterScene(const std::string& name, Args&&... args) {
        scenes_[name] = std::make_unique<T>(std::forward<Args>(args)...);
    }

    /// <summary>
    /// シーンの変更
    /// </summary>
    void ChangeScene(const std::string& name) {
        if (currentScene_) currentScene_->Finalize();
        currentScene_ = scenes_[name].get();
        if (currentScene_) currentScene_->SetManager(this), currentScene_->Init();
    }

    /// <summary>
    /// 現在のsceneの更新処理
    /// </summary>
    void Update() {
        if (currentScene_) currentScene_->Update();
    }

    /// <summary>
    /// 現在のsceneの描画処理
    /// </summary>
    void RenderBackSprite() {
        if (currentScene_) currentScene_->RenderBackSprite();
    }
    void RendRenderModeler() {
        if (currentScene_) currentScene_->RenderModel();
    }
    void RenderFrontSprite() {
        if (currentScene_) currentScene_->RenderFrontSprite();
    }


private:
	std::unordered_map<std::string, std::unique_ptr<IScene>> scenes_;
	IScene* currentScene_ = nullptr;
};

}