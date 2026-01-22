#include "RenderSystem.h"
#include <d3dx12.h>
#include "Framework/World/World.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include "Resource/ResourceSystem.h"
#include "Resource/CB/FrameCBManager.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"

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
	auto prep = world_.GetSystem<RenderPrepareSystem>();
	if (!prep) return;

	GBufferPass(prep->GetRenderPackets());
	LightingPass(prep->GetLightPacket());
}

void RenderSystem::RenderBackSprite(DX12::CommandContext&) {}

void RenderSystem::RenderModel(DX12::CommandContext&) {}

void RenderSystem::RenderFrontSprite(DX12::CommandContext&) {}

void RenderSystem::GBufferPass(const std::array<std::vector<RenderPacket>, static_cast<size_t>(SurfaceType::Count)>& lists)
{
	auto* cmd = dx12Mgr_->GetCommandContext();

	// Camera CB（GBuffer用）
	auto cameraSys = world_.GetSystem<CameraSystem>();
	const auto& cam = cameraSys->GetCameraContext();
	if (!cam.valid) return;

	GpuViewCB viewCB{};
	viewCB.view = cam.view;
	viewCB.proj = cam.proj;
	viewCB.viewProj = cam.viewProj;
	viewCB.cameraPos = cam.position;

	auto viewCBAddr = resourceSys_->GetFrameCBManager()->UploadCB(viewCB);

	// RT設定
	dx12Mgr_->SetGBufferRenderTargets(cmd);
	dx12Mgr_->ClearGBuffer();
	cmd->SetViewport(dx12Mgr_->GetMainViewport());
	cmd->SetScissor(dx12Mgr_->GetMainScissor());

	// PSO / RootSig
	cmd->GetList()->SetPipelineState(psoLib_->Get("GBuffer"));
	cmd->GetList()->SetGraphicsRootSignature(rootSigLib_->Get("GBuffer"));

	// DescriptorHeap
	ID3D12DescriptorHeap* heaps[] = { dx12Mgr_->GetPersistentDescAlloc()->GetHeap() };
	cmd->GetList()->SetDescriptorHeaps(1, heaps);

	// Opaque のみ（まずは）
	const auto& list = lists[static_cast<size_t>(SurfaceType::Opaque)];
	if (list.empty()) return;

	cmd->GetList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// RootParam[0] : ViewCB (b0)
	cmd->GetList()->SetGraphicsRootConstantBufferView(0, viewCBAddr);

	for (const auto& pkt : list)
	{
		RenderPackets(*cmd, pkt);
	}
}

void RenderSystem::LightingPass(const LightPacket& lightPacket)
{
	auto* cmd = dx12Mgr_->GetCommandContext();

	// BackBuffer
	dx12Mgr_->SetBackBufferAsRenderTarget();
	dx12Mgr_->ClearBackBuffer();
	cmd->SetViewport(dx12Mgr_->GetMainViewport());
	cmd->SetScissor(dx12Mgr_->GetMainScissor());

	// PSO / RootSig
	cmd->GetList()->SetPipelineState(psoLib_->Get("LightingDirectional"));
	cmd->GetList()->SetGraphicsRootSignature(rootSigLib_->Get("LightingDirectional"));

	// DescriptorHeap
	ID3D12DescriptorHeap* heaps[] = { dx12Mgr_->GetPersistentDescAlloc()->GetHeap() };
	cmd->GetList()->SetDescriptorHeaps(1, heaps);

	// Camera CB（Lighting用）
	auto cameraSys = world_.GetSystem<CameraSystem>();
	const auto& cam = cameraSys->GetCameraContext();
	if (!cam.valid) return;

	GpuViewCB viewCB{};
	viewCB.view = cam.view;
	viewCB.proj = cam.proj;
	viewCB.viewProj = cam.viewProj;
	viewCB.cameraPos = cam.position;

	auto camCBAddr = resourceSys_->GetFrameCBManager()->UploadCB(viewCB);

	// DirectionalLightCB
	auto lightCBAddr = resourceSys_->GetFrameCBManager()->UploadCB(lightPacket.dirCB);

	// Root params
	cmd->GetList()->SetGraphicsRootConstantBufferView(0, camCBAddr);    // b0 Camera
	cmd->GetList()->SetGraphicsRootConstantBufferView(1, lightCBAddr);  // b1 Light
	cmd->GetList()->SetGraphicsRootDescriptorTable(2, dx12Mgr_->GetGBufferSRVTable()); // t10.. etc

	// Fullscreen triangle
	cmd->GetList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->GetList()->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::RenderPackets(DX12::CommandContext& cmd, const RenderPacket& pkt)
{
	BindMesh(cmd, pkt);
	BindTransform(cmd, pkt);
	BindMaterial(cmd, pkt);

	cmd.GetList()->DrawIndexedInstanced(
		pkt.mesh->indexCount, 1, 0, 0, 0);
}

void RenderSystem::BindMesh(DX12::CommandContext& cmd, const RenderPacket& pkt)
{
	if (!pkt.mesh) return;


	if (pkt.mesh->currentState != D3D12_RESOURCE_STATE_GENERIC_READ) {
		D3D12_RESOURCE_BARRIER barriers[2];
		int count = 0;
		// VB
		barriers[count++] = CD3DX12_RESOURCE_BARRIER::Transition(
			pkt.mesh->vertexBuffer.Get(),
			pkt.mesh->currentState,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
		// IB
		barriers[count++] = CD3DX12_RESOURCE_BARRIER::Transition(
			pkt.mesh->indexBuffer.Get(),
			pkt.mesh->currentState,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
		cmd.GetList()->ResourceBarrier(count, barriers);
		pkt.mesh->currentState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	cmd.GetList()->IASetVertexBuffers(0, 1, &pkt.mesh->vbView);
	cmd.GetList()->IASetIndexBuffer(&pkt.mesh->ibView);
}

void RenderSystem::BindTransform(DX12::CommandContext& cmd, const RenderPacket& pkt)
{
	auto addr = resourceSys_->GetFrameCBManager()->UploadCB(pkt.xform);
	cmd.GetList()->SetGraphicsRootConstantBufferView(1, addr);
}

void RenderSystem::BindMaterial(DX12::CommandContext& cmd, const RenderPacket& pkt)
{
	if (!pkt.material) return;

	auto matAddr =
		resourceSys_->GetFrameCBManager()->UploadCB(pkt.material->cb);
	cmd.GetList()->SetGraphicsRootConstantBufferView(2, matAddr);

	// テクスチャバインド (albedo がなければ White を使う)
	Tsumi::Resource::TextureAsset* tex = pkt.material->albedo;
	if (!tex) {
		tex = resourceSys_->GetTextureManager()->GetTexture("White");
	}

	if (tex) {
		if (tex->currentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				tex->resource.Get(),
				tex->currentState,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			);
			cmd.GetList()->ResourceBarrier(1, &barrier);
			tex->currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		}

		cmd.GetList()->SetGraphicsRootDescriptorTable(
			3, tex->srvDesc.gpuHandle);
	}
}

void RenderSystem::DrawCommand(DX12::CommandContext& cmd, const RenderPacket& pkt)
{
	if (!pkt.mesh) return;

	cmd.GetList()->DrawIndexedInstanced(
		pkt.mesh->indexCount, 1, 0, 0, 0);
}
