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

void RenderSystem::DrawShadowPass(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	cmd, frame, prep;
}

void RenderSystem::DrawGBufferPass(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("GBuffer"));
	list->SetPipelineState(psoLib_->Get("GBuffer"));

	// CameraCB bind
	BindGBufferCamera(frame, prep);

	// Object bind & draw
	BindGBufferObjects(cmd, frame, prep);
}

void RenderSystem::DrawLightingPass(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("LightingDirectional"));
	list->SetPipelineState(psoLib_->Get("LightingDirectional"));

	// Bind
	BindLightingCommon(frame, prep);

	// Fullscreen triangle
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::DrawDebugPass(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("DebugFullScreen"));
	list->SetPipelineState(psoLib_->Get("DebugFullScreen"));

	// Bind
	BindDebugCommon(frame, prep);

	// Fullscreen triangle
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::OnResize(uint32_t w, uint32_t h)
{
	w, h;
}

void RenderSystem::SyncShadowResources()
{
}

void RenderSystem::BindGBufferCamera(DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	const CameraPacket& campck = prep.GetCameraPacket();

	auto handle = frame.UploadToTableCB(campck.camMatCB);
	frame.bind.SetTable(ToRoot(Root_GBuffer::CameraCB), handle);
}

void RenderSystem::BindGBufferObjects(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
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
		auto objHandle = frame.UploadToTableCB(pkt.xform);
		frame.bind.SetTable(ToRoot(Root_GBuffer::ObjectCB), objHandle);

		// MaterialCB (b2)
		auto matHandle = frame.UploadToTableCB(pkt.material->cb);
		frame.bind.SetTable(ToRoot(Root_GBuffer::MaterialCB), matHandle);

		// Albedo SRV table (t0)
		frame.bind.SetTable(ToRoot(Root_GBuffer::AlbedoSRV), pkt.material->albedo->srv.gpu);

		// Draw
		list->DrawIndexedInstanced(pkt.mesh->indexCount, 1, 0, 0, 0);
	}
}

void RenderSystem::BindLightingCommon(DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	// CameraCB (b0)
	const auto& camPkt = prep.GetCameraPacket();
	auto camHandle = frame.UploadToTableCB(camPkt.camMatCB);
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::CameraCB), camHandle);

	// DirLightCB (b3)
	const auto& lightPkt = prep.GetLightPacket();
	auto lightHandle = frame.UploadToTableCB(lightPkt.dirCB);
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::DirLightCB), lightHandle);

	// GBuffer table (t10..t13)
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::GBufferTable), dx12Mgr_->GetGBufferSrvTable());
}

void RenderSystem::BindDebugCommon(DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	// CameraCB 
	const auto& camPkt = prep.GetCameraPacket();
	auto camHandle = frame.UploadToTableCB(camPkt.camMatCB);
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::CameraCB), camHandle);

	// DirLightCB 
	const auto& lightPkt = prep.GetLightPacket();
	auto lightHandle = frame.UploadToTableCB(lightPkt.dirCB);
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::DirLightCB), lightHandle);

	// DebugCB 
	struct GpuDebugCB { int mode; float pad[3]; };
	GpuDebugCB dbg{}; dbg.mode = debugMode_; 
	auto dbgHandle = frame.UploadToTableCB(dbg);
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::DebugCB), dbgHandle);

	// GBuffer table
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::GBufferTable), dx12Mgr_->GetGBufferSrvTable());
}

void RenderSystem::DrawShadowCasters(DX12::CommandContext& cmd, DX12::FrameResources& frame)
{
	cmd, frame;
}
