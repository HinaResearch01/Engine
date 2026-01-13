#include "RenderSystem.h"
#include "Framework/World/World.h"
#include "DX12/DX12Manager.h"
#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"

using namespace Tsumi::Framework;

RenderSystem::RenderSystem(World& world)
	: world_(world)
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
	resourceSys_ = Resource::ResourceSystem::GetInstance();
	psoLib_ = Graphic::PSOLibrary::GetInstance();
	rootSigLib_ = Graphic::RootSignatureLibrary::GetInstance();
}

void RenderSystem::Update(float)
{
	// 1) material packet 構築
	for (auto& l : lists_) l.clear();

	// 2) draw packet 構築
	for (auto [rc, mc, tc] : world_.View<RenderComponent, MaterialComponent, TransformComponent>())
	{
		if (!rc.visible || !mc.visible) continue;
		if (rc.mesh.empty()) continue;

		//auto* mesh = meshMgr.GetMesh(rc.meshKey);
		//if (!mesh) continue;

		//auto* mat = materialSys.GetPacket(mc);
		//if (!mat) continue;

		//DrawPacket pkt{};
		//pkt.surface = mc.surface;
		//pkt.mesh = mesh;
		//pkt.material = mat;

		//// Transform → GPU構造体（あなたのTransformComponentに合わせる）
		//// TODO: tc.world を取り出して埋める
		//// pkt.xform.world = tc.world;
		//// pkt.xform.worldInvTranspose = tc.worldInvTranspose;
		//// ここはあなたの実装に合わせて確定させよう（次の段階）

		//const auto& pass = RenderPassTable::Get(pkt.surface);
		//pkt.sortKey = MakeSortKey(pkt, pass.transparent);

		//lists_[static_cast<size_t>(pkt.surface)].push_back(pkt);
	}

	SortLists();
}

void RenderSystem::RenderBackSprite(DX12::CommandContext& cmd)
{
	cmd;
}

void RenderSystem::RenderModel(DX12::CommandContext& cmd)
{
	cmd;
}

void RenderSystem::RenderFrontSprite(DX12::CommandContext& cmd)
{
	cmd;
}

void RenderSystem::SortLists()
{
	for (size_t i = 0; i < lists_.size(); ++i)
	{
/*		auto surface = static_cast<SurfaceType>(i);
		const auto& pass = RenderPassTable::Get(surface);

		auto& list = lists_[i];
		if (list.empty()) continue;

		std::sort(list.begin(), list.end(), [&](const DrawPacket& a, const DrawPacket& b)
		{
			// opaque: 状態変更最小化
			// transparent: depthが入ったら遠→近にする（今は未実装）
			return a.sortKey < b.sortKey;
		});*/
	}
}
