#include "RenderPrepareSystem.h"
#include "Framework/World/World.h"
#include "Framework/System/Light/LightSystem.h"
#include "Framework/System/Material/MaterialSystem.h"
#include "Framework/Str/RenderPassTable.h"
#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"

using namespace Tsumi::Framework;

uint64_t MakeSortKey(const RenderPacket& p, bool transparent)
{
	uint64_t a = static_cast<uint64_t>(p.surface) & 0xFF;
	uint64_t b = (reinterpret_cast<uintptr_t>(p.material) >> 4) & 0xFFFFFF;
	uint64_t c = (reinterpret_cast<uintptr_t>(p.mesh) >> 4) & 0xFFFFFF;
	uint64_t d = 0; // depth (TODO)
	transparent;
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
	BuildRenderPackets();
	BuildLightPacket();
}

void RenderPrepareSystem::Clear()
{
	for (auto& l : renderPackets_) l.clear();
	lightPacket_ = {};
}

void RenderPrepareSystem::BuildRenderPackets()
{
	auto materialSys = world_.GetSystem<MaterialSystem>();

	for (auto [rc, mc, tc] :
		 world_.View<RenderComponent, MaterialComponent, TransformComponent>())
	{
		if (!rc.visible || !mc.visible) continue;
		if (rc.mesh.empty()) continue;

		auto* mesh = resourceSys_->GetMeshManager()->GetMesh(rc.mesh);
		if (!mesh) continue;

		auto* mat = materialSys->GetPacket(mc);
		if (!mat) continue;

		RenderPacket pkt{};
		pkt.surface = mc.surface;
		pkt.mesh = mesh;
		pkt.material = mat;

		// フォールバックテクスチャ
		if (!pkt.material->albedo && !mesh->defaultTextureKey.empty()) {
			auto* tex = resourceSys_->GetTextureManager()->GetTexture(mesh->defaultTextureKey);
			if (tex) {
				const_cast<MaterialPacket*>(pkt.material)->albedo = tex;
			}
		}

		FillTransformPacket(pkt, tc);

		const auto& pass = RenderPassTable::Get(pkt.surface);
		pkt.sortKey = MakeSortKey(pkt, pass.transparent);

		renderPackets_[static_cast<size_t>(pkt.surface)].push_back(pkt);
	}

	// ソートする
	SortRenderPackets();
}

void RenderPrepareSystem::BuildLightPacket()
{
	auto lightSys = world_.GetSystem<LightSystem>();
	if (!lightSys) return;

	const auto& lc = lightSys->GetLightContext();

	// Directiona
	lightPacket_.dirCB = {};
	if (lc.directional.enabled)
	{
		lightPacket_.dirCB.directionWS = lc.directional.dirWS;
		lightPacket_.dirCB.radiance = lc.directional.radiance;
		lightPacket_.dirCB.castShadow = 1;
	}
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
