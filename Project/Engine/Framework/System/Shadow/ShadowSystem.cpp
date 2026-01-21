#include "ShadowSystem.h"
#include "Framework/World/World.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/System/Light/LightSystem.h"
#include <cmath>

using namespace Tsumi;
using namespace Framework;

static Math::Vec3f NormalizeSafe(const Math::Vec3f& v, const Math::Vec3f& fallback)
{
	const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len2 <= 1e-12f) return fallback;
	const float invLen = 1.0f / std::sqrt(len2);
	return { v.x * invLen, v.y * invLen, v.z * invLen };
}

ShadowSystem::ShadowSystem(World& world)
	: world_(world)
{}

void ShadowSystem::Update(float)
{
	
}

Math::Mat4x4 ShadowSystem::BuildOrthoDX(float halfSize, float nearZ, float farZ) const
{
	Math::Mat4x4 m =
		Math::Func::MAT4x4::OrthographicMatrix(
		-halfSize, +halfSize, -halfSize, +halfSize,
		nearZ, farZ);
	return m;
}

Math::Mat4x4 ShadowSystem::BuildLightView(const Math::Vec3f& dirWS, const Math::Vec3f& centerWS) const
{
	// ライトは中心点から逆方向に離した位置に置く
	const Math::Vec3f forward = NormalizeSafe({ -dirWS.x, -dirWS.y, -dirWS.z }, { 0,0,1 });

	const Math::Vec3f up0 = { 0,1,0 };
	Math::Vec3f right = Math::Func::VEC3::Cross(up0, forward);
	right = NormalizeSafe(right, { 1,0,0 });
	Math::Vec3f up = Math::Func::VEC3::Cross(forward, right);

	// eye = center - dir * distance
	const float distance = 100.0f;
	const Math::Vec3f eye = {
		centerWS.x - dirWS.x * distance,
		centerWS.y - dirWS.y * distance,
		centerWS.z - dirWS.z * distance
	};

	Math::Mat4x4 m =
		Math::Func::MAT4x4::LookAtLH(eye, centerWS, up);
	return m;
}
