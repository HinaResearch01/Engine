#include "LightSystem.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Light/PointLightComponent.h"
#include "Framework/Component/Light/SpotLightComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"

using namespace Tsumi;
using namespace Tsumi::Framework;

static Math::Vec3f NormalizeSafe(const Math::Vec3f& v, const Math::Vec3f& fallback)
{
	const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len2 <= 1e-12f) return fallback;
	const float invLen = 1.0f / std::sqrt(len2);
	return { v.x * invLen, v.y * invLen, v.z * invLen };
}

LightSystem::LightSystem(World& world)
	: world_(world)
{}

void LightSystem::Update(float)
{
	BuildLightContext();
}

void LightSystem::BuildLightContext()
{
	
}
