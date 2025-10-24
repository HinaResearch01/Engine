#include "MyGame.h"

#include "Test/TestScene.h"

using namespace Tsumi;

Tsumi::MyGame::MyGame()
{
	sceneMgr_ = std::make_unique<SceneManager>();
}

void MyGame::OnInit()
{
	sceneMgr_->RegisterScene<TestScene>("Test");

	sceneMgr_->ChangeScene("Test");
}

void MyGame::OnUpdate()
{
	sceneMgr_->Update();
}

void Tsumi::MyGame::OnBKSpriteRender()
{
	sceneMgr_->RenderBackSprite();
}

void MyGame::OnEntityRender()
{
	sceneMgr_->RendRenderModeler();
}

void Tsumi::MyGame::OnFTSpriteRender()
{
	sceneMgr_->RenderFrontSprite();
}

void MyGame::OnFinalize()
{
}
