#include "TestWorld.h"
#include "Loader/LoaderAPI.h"
#include "../Actor/Camera/TestCamera.h"
#include "../Actor/DirLight/TestDirLight.h"
#include "../Actor/SpotLight/TestSpotLight.h"
#include "../Actor/Axis/TestAxis.h"
#include "../Actor/Floor/TestFloor.h"
#include "../Actor/Cylinder/TestCylinder.h"
#include "../Actor/KamataCity/KamataCity.h"
#include "../Actor/LakeTown/LakeTownCity.h"
#include "../Actor/Terrain/TestTerrain.h"

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
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/laketown/laketown.obj", "laketown");
	tme::API::AssetLoader::Load<mdl>("Resources/Model/Test/Terrein/TestTerrain.obj", "TestTerrain");
	// TestActorの追加
	World::SpawnActor<TestCamera>();
	World::SpawnActor<TestDirLight>();
	World::SpawnActor<TestTerrain>();
}

void TestWorld::Update(float deltaTime)
{
	WorldUpdate(deltaTime);
}