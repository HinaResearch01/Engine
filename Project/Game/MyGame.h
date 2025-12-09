#pragma once

#include "../Engine/Core/GameApp.h"
#include "../Engine/Scene/SceneManager.h"

#include "Test/TestScene.h"

namespace  Tsumi {

/* ゲーム全体のルート制御 */
class MyGame : public GameApp {

public:
	MyGame();
    ~MyGame() = default;

    void OnInit() override;
    void OnUpdate() override;
    void OnBKSpriteRender() override;
    void OnEntityRender() override;
    void OnFTSpriteRender() override;
    void OnFinalize() override;

private:
    std::unique_ptr<SceneManager> sceneMgr_;
};


inline MyGame::MyGame()
{
	sceneMgr_ = std::make_unique<SceneManager>();
	sceneMgr_->RegisterScene<TestScene>("Test");
	sceneMgr_->ChangeScene("Test");
}

inline void MyGame::OnInit()
{
	sceneMgr_->Init();
}

inline void MyGame::OnUpdate()
{
	sceneMgr_->Update();
}

inline void MyGame::OnBKSpriteRender()
{
	sceneMgr_->RenderBackSprite();
}

inline void MyGame::OnEntityRender()
{
	sceneMgr_->RendModeler();
}

inline void MyGame::OnFTSpriteRender()
{
	sceneMgr_->RenderFrontSprite();
}

inline void MyGame::OnFinalize()
{
}

}