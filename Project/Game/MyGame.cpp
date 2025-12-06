#include "MyGame.h"

#include "Test/TestScene.h"

using namespace Tsumi;

MyGame::MyGame()
{
	sceneMgr_ = std::make_unique<SceneManager>();
	sceneMgr_->RegisterScene<TestScene>("Test");
	sceneMgr_->ChangeScene("Test");
}

void MyGame::OnInit()
{
	sceneMgr_->Init();
}

void MyGame::OnUpdate()
{
	sceneMgr_->Update();
}

void MyGame::OnBKSpriteRender()
{
	sceneMgr_->RenderBackSprite();
}

void MyGame::OnEntityRender()
{
	sceneMgr_->RendModeler();
}

void MyGame::OnFTSpriteRender()
{
	sceneMgr_->RenderFrontSprite();
}

void MyGame::OnFinalize()
{
}
