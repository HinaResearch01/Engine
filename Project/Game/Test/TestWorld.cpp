#include "TestWorld.h"
#include "imgui.h"
#include "Loader/LoaderAPI.h"

void TestWorld::Init()
{
	// テクスチャ読み込み
	tme::API::AssetLoader::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");
	tme::API::AssetLoader::Load<tex>("Resources/Texture/monsterBall.png", "monsterBall");
	// モデル読み込み
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Axis/Axis.obj", "Axis");

	// TestActorの追加
	World::SpawnActor<TestActor>();
}
