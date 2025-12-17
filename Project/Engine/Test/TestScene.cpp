#include "TestScene.h"
#include "imgui.h"
#include "Resource/ResourceAPI.h"

void TestScene::Init()
{
	tme::ResourceAPI::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");
	tme::ResourceAPI::Load<tex>("Resources/Texture/monsterBall.png", "monsterBall");
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
