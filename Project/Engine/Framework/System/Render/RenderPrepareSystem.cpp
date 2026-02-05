#include "RenderPrepareSystem.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/System/Light/LightSystem.h"
#include "Framework/System/Shadow/ShadowSystem.h"
#include "Framework/System/Material/MaterialSystem.h"
#include "Framework/Str/RenderPassTable.h"
#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"

using namespace Tsumi::Framework;

uint64_t MakeSortKey(const RenderPacket& p, bool /*transparent*/)
{
	uint64_t a = static_cast<uint64_t>(p.surface) & 0xFF;
	uint64_t b = (reinterpret_cast<uintptr_t>(p.albedo) >> 4) & 0xFFFFFF;
	uint64_t c = (reinterpret_cast<uintptr_t>(p.mesh) >> 4) & 0xFFFFFF;
	uint64_t d = 0; // depth (TODO)

	return (a << 56) | (b << 32) | (c << 8) | d;
}

RenderPrepareSystem::RenderPrepareSystem(World& world)
	: world_(world)
{
	resourceSys_ = Resource::ResourceSystem::GetInstance();
}

void RenderPrepareSystem::Update(float)
{
	Clear();
	BuildCameraPacket();
	BuildLightPacket();
	BuildShadowPacket();
	BuildRenderPackets();
}

void RenderPrepareSystem::Clear()
{
	cameraPacke_ = {};
	lightPacket_ = {};
	lightPacket_.pointCB.clear();
	lightPacket_.spotCB.clear();
	shadowPacket_ = {};
	for (auto& l : renderPackets_) l.clear();
}

void RenderPrepareSystem::BuildCameraPacket()
{
	auto cameraSys = world_.GetSystem<CameraSystem>();
	const CameraContext& ctx = cameraSys->GetContext();

	// ----- Matrices -----
	{
		cameraPacke_.camMatCB.view = ctx.view;
		cameraPacke_.camMatCB.proj = ctx.proj;
		cameraPacke_.camMatCB.viewProj = ctx.viewProj;
		cameraPacke_.camMatCB.invView = ctx.invView;
		cameraPacke_.camMatCB.invProj = ctx.invProj;
		cameraPacke_.camMatCB.invViewProj = ctx.invViewProj;
	}

	// ----- Parameter -----
	{
		cameraPacke_.camParamCB.position = ctx.position;
		cameraPacke_.camParamCB.forward = ctx.forward;
		cameraPacke_.camParamCB.fovY = ctx.fovY;
		cameraPacke_.camParamCB.aspectRatio = ctx.aspectRatio;
		cameraPacke_.camParamCB.nearPlane = ctx.nearPlane;
		cameraPacke_.camParamCB.farPlane = ctx.farPlane;
	}
}

void RenderPrepareSystem::BuildLightPacket()
{
	// LightSystem から Context を取る
	auto lightSys = world_.GetSystem<LightSystem>();
	const LightContext& ctx = lightSys->GetContext();

	// -------------------------
	// Directional
	// -------------------------
	if (ctx.directional.enabled)
	{
		lightPacket_.dirCB.enabled = 1;
		lightPacket_.dirCB.directionWS = ctx.directional.dirWS;
		lightPacket_.dirCB.radiance = ctx.directional.radiance;
	}
	else
	{
		lightPacket_.dirCB.enabled = 0;
		lightPacket_.dirCB.directionWS = { 0,-1,0 };
		lightPacket_.dirCB.radiance = { 0,0,0 };
	}

	// -------------------------
	// Point (CPU resolved -> GPU CB)
	// -------------------------
	lightPacket_.pointCB.reserve(ctx.points.size());
	for (const auto& p : ctx.points)
	{
		GpuPointLightCB cb{};
		cb.positionWS = p.positionWS;
		cb.range = p.range;
		cb.radiance = p.radiance;
		cb._pad0 = 0.0f;
		lightPacket_.pointCB.push_back(cb);
	}

	// -------------------------
	// Spot
	// -------------------------
	lightPacket_.spotCB.reserve(ctx.spots.size());
	for (const auto& s : ctx.spots)
	{
		GpuSpotLightCB cb{};
		cb.positionWS = s.positionWS;
		cb.range = s.range;
		cb.directionWS = s.directionWS;
		cb.innerCos = s.innerCos;
		cb.radiance = s.radiance;
		cb.outerCos = s.outerCos;
		lightPacket_.spotCB.push_back(cb);
	}
}

void RenderPrepareSystem::BuildShadowPacket()
{
	auto shadowSys = world_.GetSystem<ShadowSystem>();
	const ShadowContext& ctx = shadowSys->GetContext();

	auto& cb = shadowPacket_.csmCB;

	cb.enabled = ctx.enabled ? 1 : 0;
	cb.cascadeCount = static_cast<int>(ctx.cascadeCount);
	cb.shadowMapSize = static_cast<float>(ctx.shadowMapSize);
	cb.invShadowMapSize = (ctx.shadowMapSize > 0) ? (1.0f / cb.shadowMapSize) : 0.0f;

	for (int i = 0; i < 4; ++i) {
		cb.splitFar[i] = ctx.splitFar[i];
	}

	for (uint32_t i = 0; i < 4; ++i)
	{
		if (ctx.enabled && i < ctx.cascadeCount)
			cb.shadowViewProj[i] = ctx.cascades[i].viewProj;
		else
			cb.shadowViewProj[i] = Math::Mat4x4{};
	}
}

void RenderPrepareSystem::BuildRenderPackets()
{
	auto* materialSys = world_.GetSystem<MaterialSystem>();
	const auto& matCtx = materialSys->GetContext();

	for (auto [rc, mc, tc] :
		 world_.View<RenderComponent, MaterialComponent, TransformComponent>())
	{
		if (!rc.visible || !mc.visible) continue;
		if (rc.mesh.empty()) continue;

		auto* mesh = resourceSys_->GetMeshManager()->GetMesh(rc.mesh);
		if (!mesh) continue;

		MaterialKey key{ mc.surface, mc.albedo };
		const MaterialResolved* mr = matCtx.Find(key);
		if (!mr) continue;

		RenderPacket pkt{};
		pkt.surface = mc.surface;
		pkt.mesh = mesh;

		// ===== Transform =====
		pkt.xform.world = tc.world;
		pkt.xform.worldInvTranspose = tc.worldInvTranspose;

		// ===== Material =====
		pkt.materialUVCB = mr->uv;
		pkt.materialParamsCB = mr->params;
		pkt.albedo = mr->albedo;

		// fallback texture
		if (!pkt.albedo && !mesh->defaultTextureKey.empty())
		{
			pkt.albedo =
				resourceSys_->GetTextureManager()
				->GetTexture(mesh->defaultTextureKey);
		}

		pkt.castShadow = rc.castShadow;

		const auto& pass = RenderPassTable::Get(pkt.surface);
		pkt.sortKey = MakeSortKey(pkt, pass.transparent);

		renderPackets_[static_cast<size_t>(pkt.surface)]
			.push_back(pkt);
	}

	SortRenderPackets();
}

void RenderPrepareSystem::SortRenderPackets()
{
	for (size_t i = 0; i < renderPackets_.size(); ++i)
	{
		auto surface = static_cast<SurfaceType>(i);
		const auto& pass = RenderPassTable::Get(surface);
		(void)pass;

		auto& list = renderPackets_[i];
		if (list.empty()) continue;

		std::sort(list.begin(), list.end(),
				  [](const RenderPacket& a, const RenderPacket& b)
		{
			return a.sortKey < b.sortKey;
		});
	}
}

void RenderPrepareSystem::FillTransformPacket(RenderPacket& pkt, const TransformComponent& tc)
{
	pkt.xform.world = tc.world;
	pkt.xform.worldInvTranspose = tc.worldInvTranspose;
}
