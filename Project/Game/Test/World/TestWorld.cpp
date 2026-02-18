#include "TestWorld.h"
#include "Loader/LoaderAPI.h"
#include "../Actor/Camera/TestCamera.h"
#include "../Actor/DirLight/TestDirLight.h"
#include "../Actor/SpotLight/TestSpotLight.h"
#include "../Actor/Axis/TestAxis.h"
#include "../Actor/Floor/TestFloor.h"
#include "../Actor/Cylinder/TestCylinder.h"
#include "../Actor/Kamata/KamataCity.h"
#include "../Actor/LakeTown/LakeTownCity.h"

void TestWorld::Init()
{
	// テクスチャ
	tme::API::AssetLoader::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");

	// モデル
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/axis/axis.obj", "axis");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/Cube/TestCube.obj", "TestCube");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/cylinder/cylinder.obj", "cylinder");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/floor/floor.obj", "floor");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/kamata/kamata.obj", "kamata");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/laketown/laketown.obj", "laketown");

	// Actorの追加
	World::SpawnActor<TestCamera>();
	World::SpawnActor<TestDirLight>();

	World::SpawnActor<TestAxis>();
	World::SpawnActor<TestFloor>();
	World::SpawnActor<TestCylinder>();

	//World::SpawnActor<KamataCity>();
	//World::SpawnActor<LakeTownCity>();
}

void TestWorld::Update(float deltaTime)
{
	World::WorldUpdate(deltaTime);
}
