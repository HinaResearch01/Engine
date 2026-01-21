#include "LightSystem.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"

using namespace Tsumi;
using namespace Tsumi::Framework;

LightSystem::LightSystem(World& world)
	: world_(world)
{}

void LightSystem::Update(float)
{
}
