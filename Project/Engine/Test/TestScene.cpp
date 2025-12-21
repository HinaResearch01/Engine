#include "TestScene.h"
#include "imgui.h"
#include "Resource/ResourceAPI.h"

void TestScene::Init()
{
	// テクスチャ読み込み
	tme::ResourceAPI::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");
	tme::ResourceAPI::Load<tex>("Resources/Texture/monsterBall.png", "monsterBall");
	// モデル読み込み
	tme::ResourceAPI::Load<mdl>("Resources/Model/Axis/Axis.obj", "Axis");
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
