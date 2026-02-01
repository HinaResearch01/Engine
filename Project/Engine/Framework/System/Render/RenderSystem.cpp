#include "RenderSystem.h"
#include <d3dx12.h>
#include "Framework/World/World.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "DX12/DX12Manager.h"
#include "DX12/Cmd/CommandContext.h"
#include "DX12/Framebuf/Framebuffer.h"
#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"
#include "Graphic/RootSigs/RootSignatureIndex.h"

using namespace Tsumi::Framework;

RenderSystem::RenderSystem(World& world)
	: world_(world)
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
	psoLib_ = Graphic::PSOLibrary::GetInstance();
	rsLib_ = Graphic::RootSignatureLibrary::GetInstance();
}

void RenderSystem::Update(float)
{
}

void RenderSystem::RenderBackSprite(DX12::CommandContext&)
{
}

void RenderSystem::RenderModel(DX12::CommandContext& cmd)
{
	// DX12Manager から frame context を取得
	DX12::FrameContext& frame = dx12Mgr_->GetCurrentFrameContext();
	// World から RenderPrepareSystem を取得
	auto* prep = world_.GetSystem<RenderPrepareSystem>();
	assert(prep);

	// =========================================================
	// Render flow
	// =========================================================
	// 1. Shadow
	DrawShadowPass(cmd, frame, *prep);

	// 2. GBuffer
	DrawGBufferPass(cmd, frame, *prep);

	// 3. Lighting
	DrawLightingPass(cmd, frame, *prep);

	// 4. Debug (optional)
	if (debugMode_ != 0) {
		DrawDebugPass(cmd, frame, *prep);
	}
}

void RenderSystem::RenderFrontSprite(DX12::CommandContext&)
{
}

void RenderSystem::OnResize(uint32_t w, uint32_t h)
{
	w, h;
}

void RenderSystem::DrawShadowPass(DX12::CommandContext& cmd, DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	cmd, frame, prep;
}

void RenderSystem::DrawGBufferPass(DX12::CommandContext& cmd, DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// RT bind + clear は DX12Manager に寄せる
	dx12Mgr_->BeginGBufferPass();
	dx12Mgr_->ClearGBuffer();

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("GBuffer"));
	list->SetPipelineState(psoLib_->Get("GBuffer"));

	// CameraCB bind
	BindGBufferCamera(frame, prep);

	// Object bind & draw
	BindGBufferObjects(cmd, frame, prep);
}

void RenderSystem::DrawLightingPass(DX12::CommandContext& cmd, DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// RT bind + clear は DX12Manager に寄せる
	dx12Mgr_->BeginBackBufferPass();
	dx12Mgr_->ClearBackBuffer();

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("LightingDirectional"));
	list->SetPipelineState(psoLib_->Get("LightingDirectional"));

	// Bind
	BindLightingCommon(frame, prep);

	// Fullscreen triangle
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::DrawDebugPass(DX12::CommandContext& cmd, DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// RT bind は DX12Manager に寄せる
	dx12Mgr_->BeginBackBufferPass(); // 上書き

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("DebugFullScreen"));
	list->SetPipelineState(psoLib_->Get("DebugFullScreen"));

	// Bind
	BindDebugCommon(frame, prep);

	// Fullscreen triangle
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::SyncShadowResources()
{
}

void RenderSystem::BindGBufferCamera(DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	const auto& camPkt = prep.GetCameraPacket();
	const D3D12_GPU_VIRTUAL_ADDRESS camVA = frame.upload.UploadCB(camPkt.camMatCB);

	frame.bind.SetCBV(ToRoot(Root_GBuffer::CameraCB), camVA);
}

void RenderSystem::BindGBufferObjects(DX12::CommandContext& cmd, DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	auto* list = cmd.GetList();
	if (!list) return;

	// Opaqueだけ抽出
	const auto& buckets = prep.GetRenderPackets();
	const auto& gbufferList = buckets[static_cast<size_t>(SurfaceType::Opaque)];

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& pkt : gbufferList) {

		if (!pkt.mesh || !pkt.material) continue;
		if (!pkt.material->albedo) continue;

		// IA
		list->IASetVertexBuffers(0, 1, &pkt.mesh->vbView);
		list->IASetIndexBuffer(&pkt.mesh->ibView);

		// ObjectCB (b1)
		const D3D12_GPU_VIRTUAL_ADDRESS objVA = frame.upload.UploadCB(pkt.xform);
		frame.bind.SetCBV(ToRoot(Root_GBuffer::ObjectCB), objVA);

		// MaterialCB (b2)
		const D3D12_GPU_VIRTUAL_ADDRESS matVA = frame.upload.UploadCB(pkt.material->cb);
		frame.bind.SetCBV(ToRoot(Root_GBuffer::MaterialCB), matVA);

		// Albedo SRV table (t0)
		frame.bind.SetTable(ToRoot(Root_GBuffer::AlbedoSRV), pkt.material->albedo->srv.gpu);

		// Draw
		list->DrawIndexedInstanced(pkt.mesh->indexCount, 1, 0, 0, 0);
	}
}

void RenderSystem::BindLightingCommon(DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	// CameraCB (b0)
	const auto& camPkt = prep.GetCameraPacket();
	const D3D12_GPU_VIRTUAL_ADDRESS camVA = frame.upload.UploadCB(camPkt.camMatCB);
	frame.bind.SetCBV(ToRoot(Root_DirectionalLight::CameraCB), camVA);

	// DirLightCB (b3)
	const auto& lightPkt = prep.GetLightPacket();
	const D3D12_GPU_VIRTUAL_ADDRESS lightVA = frame.upload.UploadCB(lightPkt.dirCB);
	frame.bind.SetCBV(ToRoot(Root_DirectionalLight::DirLightCB), lightVA);

	// GBuffer table (t10..t13) : Global heap 上の連続 SRV
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::GBufferTable), dx12Mgr_->GetGBufferSrvTable());

}

void RenderSystem::BindDebugCommon(DX12::FrameContext& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	// CameraCB
	const auto& camPkt = prep.GetCameraPacket();
	const D3D12_GPU_VIRTUAL_ADDRESS camVA = frame.upload.UploadCB(camPkt.camMatCB);
	frame.bind.SetCBV(ToRoot(Root_DebugFullScreen::CameraCB), camVA);

	// DirLightCB
	const auto& lightPkt = prep.GetLightPacket();
	const D3D12_GPU_VIRTUAL_ADDRESS lightVA = frame.upload.UploadCB(lightPkt.dirCB);
	frame.bind.SetCBV(ToRoot(Root_DebugFullScreen::DirLightCB), lightVA);

	// DebugCB
	struct GpuDebugCB { int mode; float pad[3]; };
	GpuDebugCB dbg{}; dbg.mode = debugMode_;
	const D3D12_GPU_VIRTUAL_ADDRESS dbgVA = frame.upload.UploadCB(dbg);
	frame.bind.SetCBV(ToRoot(Root_DebugFullScreen::DebugCB), dbgVA);

	// GBuffer table
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::GBufferTable), dx12Mgr_->GetGBufferSrvTable());

}

void RenderSystem::DrawShadowCasters(DX12::CommandContext& cmd, DX12::FrameContext& frame)
{
	cmd, frame;
}
