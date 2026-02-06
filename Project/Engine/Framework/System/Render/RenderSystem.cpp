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
	auto* list = cmd.GetList();
	if (!list) return;

	const auto& shadowPkt = prep.GetShadowPacket();
	if (!shadowPkt.csmCB.enabled || shadowPkt.csmCB.cascadeCount == 0)
		return;

	// Shadow resource sync
	SyncShadowResources();

	// PSO / RootSignature
	list->SetGraphicsRootSignature(rsLib_->Get("ShadowCaster"));
	list->SetPipelineState(psoLib_->Get("ShadowCaster"));

	// Viewport / Scissor（正方形）
	const float size = static_cast<float>(shadowDMap_->GetSize());
	D3D12_VIEWPORT vp{ 0.0f, 0.0f, size, size, 0.0f, 1.0f };
	D3D12_RECT sc{ 0, 0, (LONG)size, (LONG)size };
	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);

	for (uint32_t ci = 0; ci < shadowPkt.csmCB.cascadeCount; ++ci)
	{
		// --- DSV bind & clear ---
		list->OMSetRenderTargets(
			0, nullptr, FALSE,
			shadowDMap_->GetDSVPtr()
		);

		list->ClearDepthStencilView(
			shadowDMap_->GetDSV(),
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, 0, 0, nullptr
		);

		// --- Cascade 固有 CB ---
		BindShadowCommon(frame, prep, ci);

		// --- Draw casters ---
		DrawShadowCasters(cmd, frame, prep);
	}
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
	auto shadowSys = world_.GetSystem<ShadowSystem>();
	const ShadowContext& ctx = shadowSys->GetContext();

	if (!ctx.enabled || ctx.shadowMapSize == 0)
		return;

	if (!shadowDMap_) {
		shadowDMap_ = std::make_unique<Graphic::ShadowDepthMap>();
		shadowDMap_->Init(ctx.shadowMapSize);
		cachedShadowSize_ = ctx.shadowMapSize;
		return;
	}

	if (cachedShadowSize_ != ctx.shadowMapSize) {
		shadowDMap_->Resize(ctx.shadowMapSize);
		cachedShadowSize_ = ctx.shadowMapSize;
	}
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

	const auto& buckets = prep.GetRenderPackets();
	const auto& gbufferList = buckets[static_cast<size_t>(SurfaceType::Opaque)];

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& pkt : gbufferList)
	{
		if (!pkt.mesh) continue;
		if (!pkt.albedo) continue; // フォールバック前提なら assert でもOK

		// --- IA ---
		list->IASetVertexBuffers(0, 1, &pkt.mesh->vbView);
		list->IASetIndexBuffer(&pkt.mesh->ibView);

		// --- ObjectCB ---
		auto objHandle = frame.UploadToTableCB(pkt.xform);
		frame.bind.SetTable(ToRoot(Root_GBuffer::ObjectCB), objHandle);

		// --- MaterialParamsCB ---
		auto matParamsHandle = frame.UploadToTableCB(pkt.materialParamsCB);
		frame.bind.SetTable(ToRoot(Root_GBuffer::MaterialParamsCB), matParamsHandle);

		// --- Albedo SRV ---
		if (pkt.albedo) {
			frame.bind.SetTable(ToRoot(Root_GBuffer::AlbedoSRV), pkt.albedo->srv.gpu);
		}

		// --- Draw ---
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

	// DirLightCB (b30)
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

	// DebugCB 
	struct GpuDebugCB { int mode; float scale; float pad[2]; };
	GpuDebugCB dbg{}; dbg.mode = debugMode_; dbg.scale = 1.0f;
	auto dbgHandle = frame.UploadToTableCB(dbg);
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::DebugCB), dbgHandle);

	// GBuffer table
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::GBufferTable), dx12Mgr_->GetGBufferSrvTable());

	// DirLightCB (b30) - added for Debug Lit mode
	const auto& lightPkt = prep.GetLightPacket();
	auto lightHandle = frame.UploadToTableCB(lightPkt.dirCB);
	frame.bind.SetTable(ToRoot(Root_DebugFullScreen::DirLightCB), lightHandle);
}

void RenderSystem::BindShadowCommon(DX12::FrameResources& frame, const RenderPrepareSystem& prep, uint32_t cascadeIndex)
{
	using namespace Tsumi::Graphic::RootIndex;

	const auto& shadowPkt = prep.GetShadowPacket();

	Tsumi::Framework::GpuShadowCasterCB cb{};
	cb.lightViewProj = shadowPkt.csmCB.shadowViewProj[cascadeIndex];

	auto handle = frame.UploadToTableCB(cb);
	frame.bind.SetTable(ToRoot(Root_ShadowCaster::ShadowCB), handle);
}

void RenderSystem::DrawShadowCasters(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	auto* list = cmd.GetList();
	if (!list) return;

	const auto& buckets = prep.GetRenderPackets();
	const auto& opaques = buckets[static_cast<size_t>(SurfaceType::Opaque)];

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& pkt : opaques)
	{
		if (!pkt.castShadow) continue;
		if (!pkt.mesh) continue;

		list->IASetVertexBuffers(0, 1, &pkt.mesh->vbView);
		list->IASetIndexBuffer(&pkt.mesh->ibView);

		auto objHandle = frame.UploadToTableCB(pkt.xform);
		frame.bind.SetTable(ToRoot(Root_ShadowCaster::ObjectCB), objHandle);

		list->DrawIndexedInstanced(
			pkt.mesh->indexCount, 1, 0, 0, 0
		);
	}
}
