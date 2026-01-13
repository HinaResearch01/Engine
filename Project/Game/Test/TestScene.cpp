#include "TestScene.h"
#include "imgui.h"
#include "Loader/LoaderAPI.h"

void TestScene::Init()
{
	// テクスチャ読み込み
	tme::API::AssetLoader::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");
	tme::API::AssetLoader::Load<tex>("Resources/Texture/monsterBall.png", "monsterBall");
	// モデル読み込み
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Axis/Axis.obj", "Axis");

	// TestActorの追加
	World::SpawnActor<TestActor>();
}

void TestScene::Update(float deltaTime)
{
	WorldUpdate(deltaTime);
#ifdef _DEBUG
#endif // _DEBUG
}

void TestScene::Finalize()
{
}
