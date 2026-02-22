#include "TestWorld.h"

#include "Loader/LoaderAPI.h"

#include "../Actor/Camera/TestCamera.h"
#include "../Actor/DirLight/TestDirLight.h"
#include "../Actor/SpotLight/TestSpotLight.h"
#include "../Actor/SpotLight/TestSpotLightComp.h"
#include "../Actor/Terrain/TestTerrain.h"

#include <random>
#include <ctime>

using namespace tme;

// =====================================================
// Init
// =====================================================
void TestWorld::Init()
{
	World::Init();

	LoadAssets();
	SpawnBaseActors();
	SpawnStressLights();
}

// =====================================================
// Update
// =====================================================
void TestWorld::Update(float deltaTime)
{
	WorldUpdate(deltaTime);
}

// =====================================================
// Assetロード
// =====================================================
void TestWorld::LoadAssets()
{
	using API::AssetLoader;

	// Texture
	AssetLoader::Load<tex>("Resources/Texture/uvChecker.png", "uvChecker");

	// Model
	AssetLoader::Load<mdl>("Resources/Model/Test/axis/axis.obj", "axis");
	AssetLoader::Load<mdl>("Resources/Model/Test/Cube/TestCube.obj", "TestCube");
	AssetLoader::Load<mdl>("Resources/Model/Test/cylinder/cylinder.obj", "cylinder");
	AssetLoader::Load<mdl>("Resources/Model/Test/floor/floor.obj", "floor");
	AssetLoader::Load<mdl>("Resources/Model/Test/kamata/kamata.obj", "kamata");
	AssetLoader::Load<mdl>("Resources/Model/Test/laketown/laketown.obj", "laketown");
	AssetLoader::Load<mdl>("Resources/Model/Test/Terrein/TestTerrain.obj", "TestTerrain");
}

// =====================================================
// 基本Actor
// =====================================================
void TestWorld::SpawnBaseActors()
{
	SpawnActor<TestCamera>();
	SpawnActor<TestDirLight>();
	SpawnActor<TestTerrain>();
}

// =====================================================
// ストレスライト生成
// =====================================================
void TestWorld::SpawnStressLights()
{
	std::mt19937 engine(static_cast<unsigned int>(std::time(nullptr)));

	std::uniform_real_distribution<float> posDist(-40.0f, 40.0f);
	std::uniform_real_distribution<float> colorDist(0.4f, 1.0f);
	std::uniform_real_distribution<float> heightDist(25.0f, 45.0f);

	constexpr int kLightCount = 150;
	constexpr int kShadowCount = 8;

	for (int i = 0; i < kLightCount; ++i)
	{
		auto* light = SpawnActor<TestSpotLight>();
		light->SetName("StressSpotLight_" + std::to_string(i));

		if (auto* tr = light->GetComponent<Tsumi::Framework::TransformComponent>())
		{
			tr->srt.translate = {
				posDist(engine),
				heightDist(engine),
				posDist(engine)
			};

			tr->srt.rotate = { -90.0f, 0.0f, 0.0f };
		}

		if (auto* spot = light->GetComponent<TestSpotLightComp>())
		{
			spot->color = {
				colorDist(engine),
				colorDist(engine),
				colorDist(engine)
			};

			spot->intensity = 200.0f;
			spot->range = 100.0f;
			spot->outerAngle = 2.5f;
			spot->innerAngle = 0.5f;
			spot->castShadow = (i < kShadowCount);
		}
	}
}
