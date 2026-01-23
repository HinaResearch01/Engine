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
	ctx.points.clear();
	ctx.spots.clear();

	// ============================================================
	// 1) Directional Light
	// ============================================================
	{
		for (auto [tr, dl] : world_.View<TransformComponent, DirectionalLightComponent>())
		{
			if (ctx.directional.enabled)
			{
				// 方向：Transform の forward を「ライトが照らす方向」にする
				Math::Vec3f dirWS = NormalizeSafe(tr.forward, { 0, -1, 0 });

				// 放射輝度（線形）
				Math::Vec3f radiance = {
					dl.color.x * dl.intensity,
					dl.color.y * dl.intensity,
					dl.color.z * dl.intensity
				};

				ctx.directional.enabled = true;
				ctx.directional.dirWS = dirWS;
				ctx.directional.radiance = radiance;

				// --- GPU Packet（Directional） ---
				//ctx.packet.dirCB.enabled = 1;
				//ctx.packet.dirCB.directionWS = { dirWS.x, dirWS.y, dirWS.z };
				//ctx.packet.dirCB.radiance = { radiance.x, radiance.y, radiance.z };

				break;
			}
			else if (!ctx.directional.enabled)
			{
				// GPU 側も disabled
				//ctx.packet.dirCB.enabled = 0;
				//ctx.packet.dirCB.directionWS = { 0, -1, 0 };
				//ctx.packet.dirCB.radiance = { 0, 0, 0 };

				break;
			}
		}
	}

	// ============================================================
	// 2) Point Lights（全件）
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

			// GPU Packet
			//GpuPointLightCB cb{};
			//cb.positionWS = pos;
			//cb.range = pl.range;
			//cb.radiance = radiance;
			//cb._pad0 = 0.0f;

			//ctx.packet.pointCB.push_back(cb);
		}
	}

	// ============================================================
	// 3) Spot Lights（全件）
	// ============================================================
	{
		for (auto [tr, sl] : world_.View<TransformComponent, SpotLightComponent>())
		{
			if (sl.intensity <= 0.0f || sl.range <= 0.0f)
				continue;

			Math::Vec3f pos = tr.GetWorldPos();
			Math::Vec3f dir = NormalizeSafe(tr.forward, { 0,-1,0 });

			Math::Vec3f radiance{
				sl.color.x * sl.intensity,
				sl.color.y * sl.intensity,
				sl.color.z * sl.intensity
			};

			// Resolved
			SpotLightResolved r{};
			r.positionWS = pos;
			r.range = sl.range;
			r.directionWS = dir;
			r.innerCos = sl.innerCos;
			r.outerCos = sl.outerCos;
			r.radiance = radiance;
			ctx.spots.push_back(r);

			//// GPU Packet
			//GpuSpotLightCB cb{};
			//cb.positionWS = pos;
			//cb.range = sl.range;
			//cb.directionWS = dir;
			//cb.innerCos = sl.innerCos;
			//cb.radiance = radiance;
			//cb.outerCos = sl.outerCos;

			//ctx.packet.spotCB.push_back(cb);
		}
	}

	// ============================================================
	// 4) LightContext をセット
	// ============================================================
	context_ = ctx;
}
