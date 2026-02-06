#include "TestWorld.h"
#include "imgui.h"
#include "Loader/LoaderAPI.h"

void TestWorld::Init()
{
	// テクスチャ読み込み
	tme::API::AssetLoader::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");
	// モデル読み込み
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/Cube/TestCube.obj", "TestCube");

	// TestActorの追加
	World::SpawnActor<TestActor>();
}

void TestWorld::Update(float deltaTime)
{
	World::WorldUpdate(deltaTime);
}
