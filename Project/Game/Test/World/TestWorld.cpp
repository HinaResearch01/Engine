#include "TestWorld.h"
#include "Loader/LoaderAPI.h"
#include "../Actor/Camera/TestCamera.h"
#include "../Actor/DirLight/TestDirLight.h"
#include "../Actor/SpotLight/TestSpotLight.h"
#include "../Actor/Axis/TestAxis.h"
#include "../Actor/Floor/TestFloor.h"
#include "../Actor/Cylinder/TestCylinder.h"
#include "../Actor/KamataCity/KamataCity.h"

void TestWorld::Init()
{
	World::Init();

	// テクスチャ
	tme::API::AssetLoader::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");
	
	// モデル
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/axis/axis.obj", "axis");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/Cube/TestCube.obj", "TestCube");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/cylinder/cylinder.obj", "cylinder");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/floor/floor.obj", "floor");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/kamata/kamata.obj", "kamata");
	// TestActorの追加
	World::SpawnActor<TestCamera>();
	World::SpawnActor<TestDirLight>();
	auto* spot1 = World::SpawnActor<TestSpotLight>();
	spot1->SetName("TestSpotLight 1");
	auto* spot2 = World::SpawnActor<TestSpotLight>();
	spot2->SetName("TestSpotLight 2");
	
	World::SpawnActor<TestAxis>();
	World::SpawnActor<TestCylinder>();
	World::SpawnActor<TestFloor>();

	/*auto* floor = World::SpawnActor<TestFloor>();
	auto* transComp = floor->GetComponent<Tsumi::Framework::TransformComponent>();
	transComp->srt.scale = { 100.0f, 1.0f, 100.0f };*/
	//World::SpawnActor<KamataCity>();
}

void TestWorld::Update(float deltaTime)
{
	World::WorldUpdate(deltaTime);
}
