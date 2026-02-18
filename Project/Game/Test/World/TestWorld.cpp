#include "TestWorld.h"
#include "imgui.h"
#include "Loader/LoaderAPI.h"

void TestWorld::Init()
{
	// テクスチャ読み込み
	tme::API::AssetLoader::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");
	// モデル読み込み
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/axis/axis.obj", "axis");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/Cube/TestCube.obj", "TestCube");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/cylinder/cylinder.obj", "cylinder");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/floor/floor.obj", "floor");
	// TestActorの追加
	World::SpawnActor<TestCamera>();
	World::SpawnActor<TestDirLight>();
	auto* spot1 = World::SpawnActor<TestSpotLight>();
	spot1->SetName("TestSpotLight 1");
	auto* spot2 = World::SpawnActor<TestSpotLight>();
	spot2->SetName("TestSpotLight 2");
	World::SpawnActor<TestAxis>();
	World::SpawnActor<TestFloor>();
	World::SpawnActor<TestCylinder>();
}

void TestWorld::Update(float deltaTime)
{
	World::WorldUpdate(deltaTime);
}
