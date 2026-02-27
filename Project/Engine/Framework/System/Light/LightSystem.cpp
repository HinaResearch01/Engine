#include "LightSystem.h"
#include "Framework/World/World.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Light/PointLightComponent.h"
#include "Framework/Component/Light/SpotLightComponent.h"
#include "Framework/Component/Shadow/ShadowComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/Actor/IActor.h"

using namespace Tsumi;
using namespace Tsumi::Framework;
using namespace tme;

static math::Vec3f NormalizeSafe(const math::Vec3f& v, const math::Vec3f& fallback)
{
	const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len2 <= 1e-12f) return fallback;
	const float invLen = 1.0f / std::sqrt(len2);
	return { v.x * invLen, v.y * invLen, v.z * invLen };
}

LightSystem::LightSystem(World& world)
	: ISystem(world)
{}

void LightSystem::Init()
{
	BuildDefault(defaultCtx_);
	activeCtx_ = defaultCtx_;
}

void LightSystem::Update(float)
{
	LightContext ctx{};
	BuildLightContext(ctx);

	if (!ctx.directional.enabled)
	{
		activeCtx_ = defaultCtx_;
	}
	else
	{
		activeCtx_ = ctx;
	}
}

void LightSystem::BuildLightContext(LightContext& out)
{
	// 初期化
	out = {};
	out.directional.enabled = false;

	// reserve（※ actors.size()取れるならそのまま）
	out.points.reserve(world_.GetPointLightCompView().GetActors().size());
	out.spots.reserve(world_.GetSpotLightCompView().GetActors().size());

	// ============================================================
	// 1) Directional Light（最初の1つを採用）
	// ============================================================
	{
		auto view = world_.View<TransformComponent, DirectionalLightComponent>();
		auto it = view.begin();
		if (it != view.end())
		{
			auto [tr, dl] = *it;

			math::Vec3f dirWS = NormalizeSafe(tr.world.GetForward(), { 0, -1, 0 });

			math::Vec3f radiance{
				dl.color.x * dl.intensity,
				dl.color.y * dl.intensity,
				dl.color.z * dl.intensity
			};

			out.directional.enabled = (dl.intensity > 0.0f);
			out.directional.dirWS = dirWS;
			out.directional.radiance = radiance;
			out.directional.ambient = dl.ambient;
		}
	}

	// ============================================================
	// 2) Point Lights
	// ============================================================
	for (auto [tr, pl] : world_.View<TransformComponent, PointLightComponent>())
	{
		if (pl.intensity <= 0.0f || pl.range <= 0.0f)
			continue;

		math::Vec3f pos = tr.GetWorldPos();
		math::Vec3f radiance{
			pl.color.x * pl.intensity,
			pl.color.y * pl.intensity,
			pl.color.z * pl.intensity
		};

		PointLightResolved r{};
		r.positionWS = pos;
		r.range = pl.range;
		r.radiance = radiance;

		out.points.push_back(r);
	}

	// ============================================================
	// 3) Spot Lights
	// ============================================================
	for (auto [tr, sl] : world_.View<TransformComponent, SpotLightComponent>())
	{
		if (sl.intensity <= 0.0f || sl.range <= 0.0f)
			continue;

		math::Vec3f pos = tr.GetWorldPos();
		math::Vec3f dir = NormalizeSafe(tr.forward, { 0,-1,0 });

		math::Vec3f radiance{
			sl.color.x * sl.intensity,
			sl.color.y * sl.intensity,
			sl.color.z * sl.intensity
		};

		float innerRad = math::func::NUM::ToRadians(sl.innerAngle);
		float outerRad = math::func::NUM::ToRadians(sl.outerAngle);

		SpotLightResolved r{};
		r.positionWS = pos;
		r.range = sl.range;
		r.directionWS = dir;

		// 既存コード踏襲（半角にしてcos）
		r.innerCos = std::cos(innerRad);
		r.outerCos = std::cos(outerRad);

		r.radiance = radiance;


		// Shadow
		// Default (Implicit)
		if (sl.castShadow) {
			r.shadowIndex = sl.runtimeShadowIndex;
			r.shadowBias = 0.0005f; // Default bias
		}

		// Explicit Override
		if (auto* owner = sl.GetOwner())
		{
			if (auto* shadow = owner->GetComponent<ShadowComponent>())
			{
				if (shadow->castShadow) {
					r.shadowBias = shadow->shadowBias;
				}
				else {
					if (!shadow->castShadow) {
						r.shadowIndex = -1;
					}
				}
			}
		}

		out.spots.push_back(r);
	}
}

void LightSystem::BuildDefault(LightContext& out)
{
	out = {};

	// Directional を1本だけ保証（Deferredで真っ黒回避）
	out.directional.enabled = true;

	// 太陽っぽい斜め上から
	out.directional.dirWS = NormalizeSafe({ 0.3f, -1.0f, 0.2f }, { 0, -1, 0 });

	// radiance（= color * intensity）を直に入れてる前提
	out.directional.radiance = { 1.0f, 1.0f, 1.0f };

	// Point/Spot は空でOK
	out.points.clear();
	out.spots.clear();
}