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
using namespace Tsumi::Graphic;

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

	auto shadowSys = world_.GetSystem<ShadowSystem>();
	const ShadowContext& ctx = shadowSys->GetContext();

	SyncShadowResources();

	if (shadowDMap_) {
		shadowDMap_->TransitionToWrite(cmd);
	}
	else {
		// ShadowMapがない場合は描画できない
		return;
	}

	list->SetGraphicsRootSignature(rsLib_->Get("ShadowCaster"));
	list->SetPipelineState(psoLib_->Get("ShadowCaster"));

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const float size = (float)shadowDMap_->GetSize();
	D3D12_VIEWPORT vp{ 0,0,size,size,0,1 };
	D3D12_RECT sc{ 0,0,(LONG)size,(LONG)size };

	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);

	for (uint32_t ci = 0; ci < ctx.cascadeCount; ++ci)
	{
		list->OMSetRenderTargets(0, nullptr, FALSE, shadowDMap_->GetDSVPtr(ci));

		list->ClearDepthStencilView(
			shadowDMap_->GetDSV(ci),
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, 0, 0, nullptr);

		// ShadowCB Bind
		BindShadow(frame, prep, ci);

		// TransformCB & draw
		DrawShadowCasters(cmd, frame, prep);
	}
}

void RenderSystem::DrawGBufferPass(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("DeferredGBuffer"));
	list->SetPipelineState(psoLib_->Get("DeferredGBuffer"));

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// CameraCB bind
	BindGBufferCamera(frame, prep); 

	// Object bind & draw
	BindGBufferObjects(cmd, frame, prep);
}

void RenderSystem::DrawLightingPass(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	DrawDirectionalLights(cmd, frame, prep);
	DrawPointLights(cmd, frame, prep);
	DrawSpotLights(cmd, frame, prep);
}

void RenderSystem::DrawDirectionalLights(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	auto* list = cmd.GetList();
	if (!list) return;
	if (!dx12Mgr_) return;

	// 0) States / Barriers
	// GBuffer: RT -> SRV
	dx12Mgr_->TransitionGBufferToRead();

	// Shadow: DepthWrite -> SRV
	if (shadowDMap_) {
		shadowDMap_->TransitionToRead(cmd);

		// CopyDescriptorsSimple は禁止
		// Framebufferの t4 に CreateSRV で上書きする
		dx12Mgr_->GetFramebuffer()->WriteShadowSRV(
			shadowDMap_->GetResource(),
			Graphic::CSMShadowDepthMap::kSRVFormat,
			shadowDMap_->GetCascadeCount() // Texture2DArray
		);
	}
	else {
		// shadow無しの場合：null SRV
		dx12Mgr_->GetFramebuffer()->WriteShadowSRV(
			nullptr, Graphic::CSMShadowDepthMap::kSRVFormat, 1);
	}

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("DeferredDirectionalLight"));
	list->SetPipelineState(psoLib_->Get("DeferredDirectionalLight"));

	// Bind constants + tables
	BindDirectionalLighting(frame, prep);

	// Fullscreen triangle
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::DrawPointLights(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	auto* list = cmd.GetList();
	const auto& lights = prep.GetLightPacket().pointCB;
	if (lights.empty()) return;

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("DeferredPointLight"));
	list->SetPipelineState(psoLib_->Get("DeferredPointLight"));

	// b0: Camera (Common)
	const auto& camPkt = prep.GetCameraPacket();
	frame.bind.SetTable(ToRoot(Root_PointLight::CameraCB),
						frame.UploadToTableCB(camPkt.camMatCB));

	// t0..t3: GBuffer
	frame.bind.SetTable(ToRoot(Root_PointLight::GBufferTable),
						dx12Mgr_->GetGBufferSrvTable());

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw Loop
	for (const auto& l : lights)
	{
		// b1: PointLightCB
		frame.bind.SetTable(ToRoot(Root_PointLight::PointLightCB),
							frame.UploadToTableCB(l));

		list->DrawInstanced(3, 1, 0, 0);
	}
}

void RenderSystem::DrawSpotLights(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	auto* list = cmd.GetList();
	const auto& lights = prep.GetLightPacket().spotCB;
	if (lights.empty()) return;

	// Shadow Map Transition & Bind
	if (spotShadowDMap_) {
		spotShadowDMap_->TransitionToRead(cmd);

		dx12Mgr_->GetFramebuffer()->WriteSpotShadowSRV(
			spotShadowDMap_->GetResource(),
			Graphic::SpotShadowDepthMap::kSRVFormat,
			ShadowContext::kMaxSpotShadows
		);
	}
	else {
		// No shadow map
		dx12Mgr_->GetFramebuffer()->WriteSpotShadowSRV(
			nullptr, Graphic::SpotShadowDepthMap::kSRVFormat, 1);
	}

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("DeferredSpotLight"));
	list->SetPipelineState(psoLib_->Get("DeferredSpotLight"));

	// b0: Camera (Common)
	const auto& camPkt = prep.GetCameraPacket();
	frame.bind.SetTable(ToRoot(Root_SpotLight::CameraCB),
						frame.UploadToTableCB(camPkt.camMatCB));

	// t0..t5: GBuffer Table
	frame.bind.SetTable(ToRoot(Root_SpotLight::GBufferTable),
						dx12Mgr_->GetGBufferSrvTable());

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw Loop
	for (const auto& l : lights)
	{
		// b1: SpotLightCB
		frame.bind.SetTable(ToRoot(Root_SpotLight::SpotLightCB),
							frame.UploadToTableCB(l));

		list->DrawInstanced(3, 1, 0, 0);
	}
}

void RenderSystem::DrawDebugPass(DX12::CommandContext& cmd, DX12::FrameResources& frame)
{
	auto* list = cmd.GetList();
	if (!list) return;

	// PSO / RS
	list->SetGraphicsRootSignature(rsLib_->Get("DeferredDebug"));
	list->SetPipelineState(psoLib_->Get("DeferredDebug"));

	// Bind
	BindDebug(frame);

	// Fullscreen triangle
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::SyncShadowResources()
{
	auto shadowSys = world_.GetSystem<ShadowSystem>();
	const ShadowContext& ctx = shadowSys->GetContext();

	if (!ctx.enabled || ctx.shadowMapSize == 0 || ctx.cascadeCount == 0)
		return;

	if (!shadowDMap_) {
		shadowDMap_ = std::make_unique<Graphic::CSMShadowDepthMap>();
		shadowDMap_->Init(ctx.shadowMapSize, ctx.cascadeCount);
		cachedShadowSize_ = ctx.shadowMapSize;
		cachedCascadeCount_ = ctx.cascadeCount;
		return;
	}

	if (cachedShadowSize_ != ctx.shadowMapSize || cachedCascadeCount_ != ctx.cascadeCount) {
		shadowDMap_->Resize(ctx.shadowMapSize, ctx.cascadeCount);
		cachedShadowSize_ = ctx.shadowMapSize;
		cachedCascadeCount_ = ctx.cascadeCount;
	}

	// Spot Shadow
	if (ctx.spotEnabled && ctx.spotShadowMapSize > 0 && ctx.activeSpotShadowCount > 0)
	{
		if (!spotShadowDMap_) {
			spotShadowDMap_ = std::make_unique<Graphic::SpotShadowDepthMap>();
			spotShadowDMap_->Init(ctx.spotShadowMapSize, ShadowContext::kMaxSpotShadows);
			cachedSpotShadowSize_ = ctx.spotShadowMapSize;
			cachedSpotCount_ = ShadowContext::kMaxSpotShadows;
		}
		else if (cachedSpotShadowSize_ != ctx.spotShadowMapSize) {
			spotShadowDMap_->Resize(ctx.spotShadowMapSize, ShadowContext::kMaxSpotShadows);
			cachedSpotShadowSize_ = ctx.spotShadowMapSize;
		}
	}
}

void RenderSystem::DrawSpotShadowPass(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	auto* list = cmd.GetList();
	if (!list) return;

	auto shadowSys = world_.GetSystem<ShadowSystem>();
	const ShadowContext& ctx = shadowSys->GetContext();

	SyncShadowResources();

	if (!spotShadowDMap_ || !ctx.spotEnabled) return;

	spotShadowDMap_->TransitionToWrite(cmd);

	// Reuse ShadowCaster PSO/RS (same vertex layout, depth write only)
	list->SetGraphicsRootSignature(rsLib_->Get("ShadowCaster"));
	list->SetPipelineState(psoLib_->Get("ShadowCaster"));

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const float size = (float)spotShadowDMap_->GetSize();
	D3D12_VIEWPORT vp{ 0,0,size,size,0,1 };
	D3D12_RECT sc{ 0,0,(LONG)size,(LONG)size };

	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);

	// Iterate over active spot shadows
	for (uint32_t i = 0; i < ctx.activeSpotShadowCount; ++i)
	{
		const auto& data = ctx.spotShadows[i];

		list->OMSetRenderTargets(0, nullptr, FALSE, spotShadowDMap_->GetDSVPtr(i));
		list->ClearDepthStencilView(
			spotShadowDMap_->GetDSV(i),
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, 0, 0, nullptr);

		Tsumi::Framework::GpuShadowCSMCB cb{};
		cb.lightViewProj[0] = data.viewProj;
		
		auto shadowHandle = frame.UploadToTableCB(cb);
		frame.bind.SetTable(RootIndex::ToRoot(RootIndex::Root_ShadowCaster::ShadowCB), shadowHandle);

		// CascadeIndex = 0
		struct CascadeIndexCB { uint32_t gCascadeIndex; float _pad[3]; };
		CascadeIndexCB ci{ 0 };
		auto ciHandle = frame.UploadToTableCB(ci);
		frame.bind.SetTable(RootIndex::ToRoot(RootIndex::Root_ShadowCaster::CascadeIndexCB), ciHandle);

		DrawShadowCasters(cmd, frame, prep);
	}
}

void RenderSystem::BindGBufferCamera(DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace RootIndex;
	auto h = frame.UploadToTableCB(prep.GetCameraPacket().camMatCB);
	frame.bind.SetTable(ToRoot(Root_GBuffer::CameraCB), h);
}

void RenderSystem::BindGBufferObjects(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace RootIndex;

	auto* list = cmd.GetList();

	for (const auto& pkt : prep.GetRenderPackets()[0])
	{
		list->IASetVertexBuffers(0, 1, &pkt.mesh->vbView);
		list->IASetIndexBuffer(&pkt.mesh->ibView);

		frame.bind.SetTable(
			ToRoot(Root_GBuffer::TransformCB),
			frame.UploadToTableCB(pkt.xform));

		frame.bind.SetTable(
			ToRoot(Root_GBuffer::MaterialParamsCB),
			frame.UploadToTableCB(pkt.materialParamsCB));

		frame.bind.SetTable(
			ToRoot(Root_GBuffer::AlbedoSRV),
			pkt.albedo->srv.gpu);

		list->DrawIndexedInstanced(pkt.mesh->indexCount, 1, 0, 0, 0);
	}
}

void RenderSystem::BindDirectionalLighting(DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace Tsumi::Graphic::RootIndex;

	// CameraCB (b0)
	const auto& camPkt = prep.GetCameraPacket();
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::CameraCB),
						frame.UploadToTableCB(camPkt.camMatCB));

	// DirLightCB (b1)
	const auto& lightPkt = prep.GetLightPacket();
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::DirLightCB),
						frame.UploadToTableCB(lightPkt.dirCB));

	// ShadowCB (b2)
	const auto& shadowPkt = prep.GetShadowPacket();
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::ShadowCB),
						frame.UploadToTableCB(shadowPkt.csmCB));

	// GBuffer table (t0..t4)
	// t0 Albedo / t1 Normal / t2 Material / t3 Depth / t4 Shadow
	frame.bind.SetTable(ToRoot(Root_DirectionalLight::GBufferTable),
						dx12Mgr_->GetGBufferSrvTable());
}

void RenderSystem::BindDebug(DX12::FrameResources& frame)
{
	using namespace RootIndex;

	struct DebugCB { int mode; float scale; float pad[2]; };
	DebugCB cb{ debugMode_, 1.0f };

	// DebugCB
	frame.bind.SetTable(
		ToRoot(Root_DeferredDebug::DebugCB),
		frame.UploadToTableCB(cb)
	);

	// GBuffer
	frame.bind.SetTable(
		ToRoot(Root_DeferredDebug::GBufferTable),
		dx12Mgr_->GetGBufferSrvTable()
	);
}

void RenderSystem::BindShadow(DX12::FrameResources& frame, const RenderPrepareSystem& prep, uint32_t cascadeIndex)
{
	using namespace Tsumi::Graphic::RootIndex;

	// b1: ShadowCB
	auto shadowHandle = frame.UploadToTableCB(prep.GetShadowPacket().csmCB);
	frame.bind.SetTable(ToRoot(Root_ShadowCaster::ShadowCB), shadowHandle);

	// b2: CascadeIndexCB
	struct CascadeIndexCB
	{
		uint32_t gCascadeIndex;
		float _pad[3];
	};

	CascadeIndexCB cb{};
	cb.gCascadeIndex = cascadeIndex;

	auto ciHandle = frame.UploadToTableCB(cb);
	frame.bind.SetTable(ToRoot(Root_ShadowCaster::CascadeIndexCB), ciHandle);
}

void RenderSystem::DrawShadowCasters(DX12::CommandContext& cmd, DX12::FrameResources& frame, const RenderPrepareSystem& prep)
{
	using namespace RootIndex;

	auto* list = cmd.GetList();

	for (const auto& pkt : prep.GetRenderPackets()[0])
	{
		if (!pkt.castShadow) continue;

		list->IASetVertexBuffers(0, 1, &pkt.mesh->vbView);
		list->IASetIndexBuffer(&pkt.mesh->ibView);

		frame.bind.SetTable(
			ToRoot(Root_ShadowCaster::TransformCB),
			frame.UploadToTableCB(pkt.xform));

		list->DrawIndexedInstanced(pkt.mesh->indexCount, 1, 0, 0, 0);
	}
}
