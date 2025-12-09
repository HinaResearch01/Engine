#include "TestScene.h"
#include "imgui.h"
#include "Resource/ResourceAPI.h"

void TestScene::Init()
{
	Tsumi::ResourceAPI::Load<tex>("Resources/Texture", "uvChecker.png");
	Tsumi::ResourceAPI::Load<tex>("Resources/Texture", "monsterBall.png");
}

void TestScene::Update()
{

#ifdef _DEBUG
	
#endif // _DEBUG
}

void TestScene::RenderModel()
{
}

void TestScene::Finalize()
{
}
