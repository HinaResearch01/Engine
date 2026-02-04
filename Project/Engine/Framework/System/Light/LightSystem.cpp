#include "LightSystem.h"
#include "Framework/World/World.h"
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
{
}

void LightSystem::Update(float)
{
	BuildLightContext();
}

void LightSystem::BuildLightContext()
{
	// まず初期化
	LightContext ctx{};
	ctx.directional.enabled = false;
	ctx.points.reserve(world_.GetPointLightCompView().GetActors().size());
	ctx.spots.reserve(world_.GetSpotLightCompView().GetActors().size());

	// ============================================================
	// 1) Directional Light
	// ============================================================
	{
		for (auto [tr, dl] : world_.View<TransformComponent, DirectionalLightComponent>())
		{
			// 最初の1個だけ使う
			Math::Vec3f dirWS = NormalizeSafe(-tr.forward, { 0, -1, 0 });

			Math::Vec3f radiance{
				dl.color.x * dl.intensity,
				dl.color.y * dl.intensity,
				dl.color.z * dl.intensity
			};

			ctx.directional.enabled = true;
			ctx.directional.dirWS = dirWS;
			ctx.directional.radiance = radiance;

			break;
		}
	}

	// ============================================================
	// 2) Point Lights
	// ============================================================
	{
		for (auto [tr, pl] : world_.View<TransformComponent, PointLightComponent>())
		{
			if (pl.intensity <= 0.0f || pl.range <= 0.0f)
				continue;

			Math::Vec3f pos = tr.GetWorldPos();
			Math::Vec3f radiance{
				pl.color.x * pl.intensity,
				pl.color.y * pl.intensity,
				pl.color.z * pl.intensity
			};

			// Resolved（CPU 側）
			PointLightResolved r{};
			r.positionWS = pos;
			r.range = pl.range;
			r.radiance = radiance;
			ctx.points.push_back(r);
		}
	}

	// ============================================================
	// 3) Spot Lights
	// ============================================================
	{
		for (auto [tr, sl] : world_.View<TransformComponent, SpotLightComponent>())
		{
			if (sl.intensity <= 0.0f || sl.range <= 0.0f)
				continue;

			Math::Vec3f pos = tr.GetWorldPos();
			Math::Vec3f dir = NormalizeSafe(-tr.forward, { 0,-1,0 });

			Math::Vec3f radiance{
				sl.color.x * sl.intensity,
				sl.color.y * sl.intensity,
				sl.color.z * sl.intensity
			};

			float innerRad = Math::Func::NUM::ToRadians(sl.innerAngle);
			float outerRad = Math::Func::NUM::ToRadians(sl.outerAngle);

			SpotLightResolved r{};
			r.positionWS = pos;
			r.range = sl.range;
			r.directionWS = dir;
			r.innerCos = std::cos(innerRad * 0.5f);
			r.outerCos = std::cos(outerRad * 0.5f);
			r.radiance = radiance;

			ctx.spots.push_back(r);
		}
	}

	// ============================================================
	// 4) LightContext をセット
	// ============================================================
	context_ = ctx;
}
