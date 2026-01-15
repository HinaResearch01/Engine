#include "RenderSystem.h"
#include "Framework/World/World.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include "Resource/ResourceSystem.h"
#include "Resource/CB/FrameCBManager.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"

using namespace Tsumi::Framework;

uint64_t MakeSortKey(const DrawPacket& p, bool transparent)
{
	uint64_t a = static_cast<uint64_t>(p.surface) & 0xFF;
	uint64_t b = (reinterpret_cast<uintptr_t>(p.material) >> 4) & 0xFFFFFF;
	uint64_t c = (reinterpret_cast<uintptr_t>(p.mesh) >> 4) & 0xFFFFFF;
	uint64_t d = 0; // depth (TODO)
	transparent;
	return (a << 56) | (b << 32) | (c << 8) | d;
}

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
	ClearLists();

	// パケットを構築し、ソート処理
	BuildDrawPackets();
	SortLists();
}

void RenderSystem::RenderBackSprite(DX12::CommandContext& cmd)
{
	cmd;
}

void RenderSystem::RenderModel(DX12::CommandContext& cmd)
{
	auto viewCBAddr = UploadViewCB();
	if (viewCBAddr == 0) return;

	for (size_t i = 0; i < lists_.size(); ++i)
	{
		if (lists_[i].empty()) continue;
		RenderSurfacePass(cmd, static_cast<SurfaceType>(i), viewCBAddr);
	}
}

void RenderSystem::RenderFrontSprite(DX12::CommandContext& cmd)
{
	cmd;
}

void RenderSystem::BuildDrawPackets()
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

		DrawPacket pkt{};
		pkt.surface = mc.surface;
		pkt.mesh = mesh;
		pkt.material = mat;

		FillTransformPacket(pkt, tc);

		const auto& pass = RenderPassTable::Get(pkt.surface);
		pkt.sortKey = MakeSortKey(pkt, pass.transparent);

		lists_[static_cast<size_t>(pkt.surface)].push_back(pkt);
	}
}

void RenderSystem::FillTransformPacket(DrawPacket& pkt, const TransformComponent& tc)
{
	pkt.xform.world = tc.world;
	pkt.xform.worldInvTranspose = tc.world;
}

void RenderSystem::ClearLists()
{
	for (auto& l : lists_) {
		l.clear();
	}
}

void RenderSystem::SortLists()
{
	for (size_t i = 0; i < lists_.size(); ++i)
	{
		auto surface = static_cast<SurfaceType>(i);
		const auto& pass = RenderPassTable::Get(surface);
		pass;

		auto& list = lists_[i];
		if (list.empty()) continue;

		std::sort(list.begin(), list.end(), [&](const DrawPacket& a, const DrawPacket& b)
		{
			// opaque: 状態変更最小化
			// transparent: depthが入ったら遠→近にする（TODO 今は未実装）
			return a.sortKey < b.sortKey;
		});
	}
}

D3D12_GPU_VIRTUAL_ADDRESS RenderSystem::UploadViewCB()
{
	auto cameraSys = world_.GetSystem<CameraSystem>();
	const auto& cam = cameraSys->GetCameraContext();
	if (!cam.valid) return 0; // または描画スキップ

	GpuViewCB cb{};
	cb.view = cam.view;
	cb.proj = cam.proj;
	cb.viewProj = cam.viewProj;
	cb.cameraPos = cam.position;

	return resourceSys_->GetFrameCBManager()->UploadCB(cb);
}

void RenderSystem::RenderSurfacePass(DX12::CommandContext& cmd, SurfaceType surface, D3D12_GPU_VIRTUAL_ADDRESS viewCBAddr)
{
	const auto& pass = RenderPassTable::Get(surface);
	auto& list = lists_[static_cast<size_t>(surface)];

	SetupPassState(cmd, pass, viewCBAddr);

	for (auto& pkt : list) {
		RenderPacket(cmd, pkt);
	}
}

void RenderSystem::SetupPassState(DX12::CommandContext& cmd, const RenderPassDesc& pass, D3D12_GPU_VIRTUAL_ADDRESS viewCBAddr)
{
	cmd.GetList()->SetGraphicsRootSignature(
		rootSigLib_->Get(pass.rootName.data()));
	cmd.GetList()->SetPipelineState(
		psoLib_->Get(pass.psoName.data()));
	cmd.GetList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// RootParam[0] : ViewCB
	cmd.GetList()->SetGraphicsRootConstantBufferView(0, viewCBAddr);
}

void RenderSystem::RenderPacket(DX12::CommandContext& cmd, const DrawPacket& pkt)
{
	BindMesh(cmd, pkt);
	BindTransform(cmd, pkt);
	BindMaterial(cmd, pkt);
	DrawCommand(cmd, pkt);
}

void RenderSystem::BindMesh(DX12::CommandContext& cmd, const DrawPacket& pkt)
{
	if (!pkt.mesh) return;

	cmd.GetList()->IASetVertexBuffers(0, 1, &pkt.mesh->vbView);
	cmd.GetList()->IASetIndexBuffer(&pkt.mesh->ibView);
}

void RenderSystem::BindTransform(DX12::CommandContext& cmd, const DrawPacket& pkt)
{
	auto addr = resourceSys_->GetFrameCBManager()->UploadCB(pkt.xform);
	cmd.GetList()->SetGraphicsRootConstantBufferView(1, addr);
}

void RenderSystem::BindMaterial(DX12::CommandContext& cmd, const DrawPacket& pkt)
{
	if (!pkt.material) return;

	auto matAddr =
		resourceSys_->GetFrameCBManager()->UploadCB(pkt.material->cb);
	cmd.GetList()->SetGraphicsRootConstantBufferView(2, matAddr);

	if (pkt.material->albedo) {
		cmd.GetList()->SetGraphicsRootDescriptorTable(
			3, pkt.material->albedo->srvDesc.gpuHandle);
	}
}

void RenderSystem::DrawCommand(DX12::CommandContext& cmd, const DrawPacket& pkt)
{
	if (!pkt.mesh) return;

	cmd.GetList()->DrawIndexedInstanced(
		pkt.mesh->indexCount, 1, 0, 0, 0);
}
