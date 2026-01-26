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
#include "Resource/GView/GpuViewManager.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"

using namespace Tsumi::Framework;

RenderSystem::RenderSystem(World& world)
	: world_(world)
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
	resourceSys_ = Resource::ResourceSystem::GetInstance();
	psoLib_ = Graphic::PSOLibrary::GetInstance();
	rsLib_ = Graphic::RootSignatureLibrary::GetInstance();
}

void RenderSystem::Update(float)
{

}

void RenderSystem::RenderBackSprite(DX12::CommandContext&) {}

void RenderSystem::RenderModel(DX12::CommandContext&)
{
	auto* cmd = dx12Mgr_->GetCommandContext();
	assert(cmd);

	// =========================================================
	// 1) Shadow pass
	// =========================================================
	DrawShadowPass(cmd);

	// =========================================================
	// 2) GBuffer pass
	// =========================================================
	DrawGBufferPass(cmd);

	// =========================================================
	// 3) Lighting pass
	// =========================================================
	DrawLightingPass(cmd);

	// =========================================================
	// 4) Debug pass (optional)
	// =========================================================
	if (debugMode_ != 0)
		DrawDebugPass(cmd);
}

void RenderSystem::RenderFrontSprite(DX12::CommandContext&) {}

void RenderSystem::OnResize(uint32_t w, uint32_t h)
{
	w, h;
	// Framebuffer のリサイズが走った後に呼ばれる想定
	// Shadow は ShadowMapSize で管理
	// GBuffer SRV は Framebuffer 側が作っているなら何もしなくてよい
}

void RenderSystem::DrawShadowPass(DX12::CommandContext* cmd)
{
	auto* shSys = world_.GetSystem<ShadowSystem>();
	assert(shSys);

	const ShadowContext& sh = shSys->GetContext();
	if (!sh.enabled)
		return;

	// ensure ShadowDepthMap / SRV / DSV
	SyncShadowResources();

	auto* list = cmd->GetList();
	assert(list);

	// Viewport/Scissor to shadow size
	const float sz = (float)shadowDMap_->GetSize();

	D3D12_VIEWPORT vp{};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = sz;
	vp.Height = sz;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	D3D12_RECT rc{};
	rc.left = 0;
	rc.top = 0;
	rc.right = (LONG)shadowDMap_->GetSize();
	rc.bottom = (LONG)shadowDMap_->GetSize();

	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &rc);

	// DSV only
	list->OMSetRenderTargets(0, nullptr, FALSE, &shadowDsv_);
	list->ClearDepthStencilView(shadowDsv_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// PSO/RS (ShadowCaster)
	// NOTE: これらは作っておくこと
	// - RootSignature: "ShadowCaster" (例：b0=ShadowCasterCB, b1=ObjectCB)
	// - PSO: "ShadowCaster" (Depth only)
	list->SetGraphicsRootSignature(rsLib_->Get("ShadowCaster"));
	list->SetPipelineState(psoLib_->Get("ShadowCaster"));

	// cascade 0 をまず描く（CSM array化は次ステップ）
	{
		GpuShadowCasterCB shadowCB{};
		shadowCB.lightViewProj = sh.cascades[0].viewProj;

		// b0 を descriptor table で受ける設計なら UploadCBAndCreateView
		auto cbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(shadowCB);

		// RootParam index は RootSignature の定義順に合わせる
		// 例: [0]=b0 ShadowCasterCB, [1]=b1 ObjectCB
		list->SetGraphicsRootDescriptorTable(0, cbv.gpuHandle);
	}

	DrawShadowCasters(cmd);
}

void RenderSystem::DrawGBufferPass(DX12::CommandContext* cmd)
{
	auto* list = cmd->GetList();
	assert(list);

	// Framebuffer へ GBuffer RT セット + Clear
	dx12Mgr_->SetGBufferRenderTargets(cmd);
	dx12Mgr_->ClearGBuffer();

	// viewport/scissor = main
	auto vp = dx12Mgr_->GetMainViewport();
	auto sc = dx12Mgr_->GetMainScissor();
	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);

	// PSO/RS
	list->SetGraphicsRootSignature(rsLib_->Get("GBuffer"));
	list->SetPipelineState(psoLib_->Get("GBuffer"));

	BindGBufferCommon(cmd);
	DrawGBufferObjects(cmd);
}

void RenderSystem::DrawLightingPass(DX12::CommandContext* cmd)
{
	auto* list = cmd->GetList();
	assert(list);

	// BackBuffer RT
	dx12Mgr_->SetBackBufferAsRenderTarget();
	dx12Mgr_->ClearBackBuffer();

	// viewport/scissor main
	auto vp = dx12Mgr_->GetMainViewport();
	auto sc = dx12Mgr_->GetMainScissor();
	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);

	// PSO/RS
	list->SetGraphicsRootSignature(rsLib_->Get("LightingDirectional"));
	list->SetPipelineState(psoLib_->Get("LightingDirectional"));

	BindLightingCommon(cmd);

	// Fullscreen triangle
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::DrawDebugPass(DX12::CommandContext* cmd)
{
	auto* list = cmd->GetList();
	assert(list);

	// BackBuffer 上に上書きする想定
	dx12Mgr_->SetBackBufferAsRenderTarget();

	auto vp = dx12Mgr_->GetMainViewport();
	auto sc = dx12Mgr_->GetMainScissor();
	list->RSSetViewports(1, &vp);
	list->RSSetScissorRects(1, &sc);

	list->SetGraphicsRootSignature(rsLib_->Get("DebugFullScreen"));
	list->SetPipelineState(psoLib_->Get("DebugFullScreen"));

	BindDebugCommon(cmd);

	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->DrawInstanced(3, 1, 0, 0);
}

void RenderSystem::SyncShadowResources()
{
	auto* shSys = world_.GetSystem<ShadowSystem>();
	assert(shSys);

	const ShadowContext& sh = shSys->GetContext();
	if (!sh.enabled)
		return;

	const uint32_t wantSize = sh.shadowMapSize;
	if (!shadowDMap_)
	{
		shadowDMap_ = std::make_unique<Graphic::ShadowDepthMap>();
		shadowDMap_->Init(wantSize);
		cachedShadowSize_ = wantSize;

		CreateShadowDSV();

		// SRV登録（persistent）
		RegisterShadowSRV();
		return;
	}

	if (cachedShadowSize_ != wantSize)
	{
		shadowDMap_->Resize(wantSize);
		cachedShadowSize_ = wantSize;

		CreateShadowDSV();

		// NOTE:
		// ここで "ShadowMap" のSRVを作り直す必要がある。
		// 今の GpuViewManager に Unregister が無いなら Clear → 再登録運用。
		// Clear すると GBuffer の登録も消えるので、
		// 「GpuViewManager に Unregister(name) を追加」するのが最終的に正解。
		//
		// ここでは最小で Clear→再登録にしておく。
		resourceSys_->GetGpuViewManager()->Clear();
		RegisterShadowSRV();
		// + 必要なら GBuffer などもここで登録し直す（Framebufが自前でSRV table持つなら不要）
	}
}

void RenderSystem::RegisterShadowSRV()
{
	assert(shadowDMap_);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// Depth D32 -> SRV R32
	srv.Format = Graphic::ShadowDepthMap::kSRVFormat; // R32_FLOAT
	srv.Texture2D.MipLevels = 1;
	srv.Texture2D.MostDetailedMip = 0;
	srv.Texture2D.ResourceMinLODClamp = 0.0f;

	resourceSys_->GetGpuViewManager()->
		RegisterTextureSRV("ShadowMap", shadowDMap_->GetResource(), srv);
}

void RenderSystem::CreateShadowDSV()
{
	auto* device = dx12Mgr_->GetDevice();
	assert(device && shadowDMap_);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	shadowDsvHeap_.Reset();
	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&shadowDsvHeap_));
	assert(SUCCEEDED(hr) && shadowDsvHeap_);

	shadowDsv_ = shadowDsvHeap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
	dsv.Format = Graphic::ShadowDepthMap::kDSVFormat; // D32
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(shadowDMap_->GetResource(), &dsv, shadowDsv_);
}

void RenderSystem::BindGBufferCommon(DX12::CommandContext* cmd)
{
	auto* list = cmd->GetList();
	assert(list);

	// CameraContext → CameraCB(b0)
	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();
	assert(cam.valid);

	GpuCameraCB camCB{};
	camCB.view = cam.view;
	camCB.proj = cam.proj;
	camCB.viewProj = cam.viewProj;
	camCB.invView = cam.view.Inverse();
	camCB.invProj = cam.proj.Inverse();
	camCB.invViewProj = cam.viewProj.Inverse();
	auto camCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(camCB);

	// RootSignature "GBuffer" の定義順に合わせる
	// 例: [0]=b0 CameraCB(VS), [1]=b1 ObjectCB(VS), [2]=b2 MaterialCB(PS), [3]=t0 AlbedoTex(PS)
	list->SetGraphicsRootDescriptorTable(0, camCbv.gpuHandle);

	// t0 (AlbedoTex) は「TextureManager が持つ SRV」をここで bind する
	// ※ objectごとに material/texture が違うので DrawGBufferObjects でやる
}

void RenderSystem::BindLightingCommon(DX12::CommandContext* cmd)
{
	auto* list = cmd->GetList();
	assert(list);

	// CameraCB(b0) : PS
	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();
	assert(cam.valid);

	GpuCameraCB camCB{};
	camCB.view = cam.view;
	camCB.proj = cam.proj;
	camCB.viewProj = cam.viewProj;
	camCB.invView = cam.view.Inverse();
	camCB.invProj = cam.proj.Inverse();
	camCB.invViewProj = cam.viewProj.Inverse();
	auto camCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(camCB);

	// DirectionalLightCB(b3) : PS
	auto lightSys = world_.GetSystem<LightSystem>();
	const LightContext& lc = lightSys->GetContext();

	GpuDirectionalLightCB dir{};
	if (lc.directional.enabled) {
		dir.enabled = 1;
		dir.directionWS = lc.directional.dirWS;
		dir.radiance = lc.directional.radiance;
	}
	else {
		dir.enabled = 0;
		dir.directionWS = { 0,-1,0 };
		dir.radiance = { 0,0,0 };
	}
	auto dirCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(dir);

	// GBuffer SRV table (t10..t13)
	// 君の DX12Manager が GPU handle table を返すのでそれを使う
	D3D12_GPU_DESCRIPTOR_HANDLE gbufferTable = dx12Mgr_->GetGBufferSRVTable();

	// RootSignature "LightingDirectional" の定義順に合わせる
	// 例: [0]=b0 CameraCB, [1]=b3 DirectionalLightCB, [2]=t10..t13 table
	list->SetGraphicsRootDescriptorTable(0, camCbv.gpuHandle);
	list->SetGraphicsRootDescriptorTable(1, dirCbv.gpuHandle);
	list->SetGraphicsRootDescriptorTable(2, gbufferTable);

	// ShadowMap をここで使うなら RootSignature を拡張して t30 を追加して bind する
	// まだ "LightingDirectional" の RS に影が無いなら次ステップ
}

void RenderSystem::BindDebugCommon(DX12::CommandContext* cmd)
{
	auto* list = cmd->GetList();
	assert(list);

	// CameraCB(b0)
	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();
	assert(cam.valid);

	GpuCameraCB camCB{};
	camCB.view = cam.view;
	camCB.proj = cam.proj;
	camCB.viewProj = cam.viewProj;
	camCB.invView = cam.view.Inverse();
	camCB.invProj = cam.proj.Inverse();
	camCB.invViewProj = cam.viewProj.Inverse();
	auto camCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(camCB);

	// DirectionalLightCB(b3)（Debugで使わないなら0でもOK）
	auto lightSys = world_.GetSystem<LightSystem>();
	const LightContext& lc = lightSys->GetContext();

	GpuDirectionalLightCB dir{};
	if (lc.directional.enabled) {
		dir.enabled = 1;
		dir.directionWS = lc.directional.dirWS;
		dir.radiance = lc.directional.radiance;
	}
	auto dirCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(dir);

	// DebugCB(b4) ※ RootSig で b4 を DebugCB に割り当てた想定
	GpuDebugCB dbg{};
	dbg.mode = debugMode_;
	auto dbgCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(dbg);

	// SRV table t10..t13 (GBuffer)
	D3D12_GPU_DESCRIPTOR_HANDLE gbufferTable = dx12Mgr_->GetGBufferSRVTable();

	// Shadow SRV t30 を使うなら RootSig に追加して bind
	// 例: list->SetGraphicsRootDescriptorTable(?, gpuViewMgr_.GetSRV("ShadowMap"));
	// 今は DebugFullScreen の RootSig 仕様次第なのでコメントに留める

	// RootSignature "DebugFullScreen" の定義順に合わせる
	// 例: [0]=b0 Camera, [1]=b3 DirLight, [2]=b4 Debug, [3]=t10..t13 table
	list->SetGraphicsRootDescriptorTable(0, camCbv.gpuHandle);
	list->SetGraphicsRootDescriptorTable(1, dirCbv.gpuHandle);
	list->SetGraphicsRootDescriptorTable(2, dbgCbv.gpuHandle);
	list->SetGraphicsRootDescriptorTable(3, gbufferTable);
}

void RenderSystem::DrawShadowCasters(DX12::CommandContext* cmd)
{
	// ---------------------------------------------------------
	// ここは君の既存 RenderSystem の「Objectを回す」ロジックに置換する
	// - RenderComponent (mesh/material)
	// - TransformComponent
	// - MeshManager から VB/IB
	// - IA Set
	// - ObjectCB (world) を b1 に bind
	// ---------------------------------------------------------
	auto* list = cmd->GetList();
	assert(list);

	// 例：疑似コード
	// for (auto [tr, render] : world_.View<TransformComponent, RenderComponent>()) {
	//    GpuObjectCB obj{}; obj.gWorld = tr.world;
	//    auto objCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(obj);
	//    list->SetGraphicsRootDescriptorTable(1, objCbv.gpuHandle);
	//
	//    BindMeshIA(render.mesh);
	//    list->DrawIndexedInstanced(...);
	// }

	// 今は「実体は君の既存コードに差し替え」前提
}

void RenderSystem::DrawGBufferObjects(DX12::CommandContext* cmd)
{
	// ---------------------------------------------------------
	// ここも君の既存 RenderSystem の DrawPacket/ソートを流用する
	// 典型：
	// - ObjectCB(b1) : world
	// - MaterialCB(b2)
	// - SRV(t0) : Albedo (TextureManager)
	// ---------------------------------------------------------
	auto* list = cmd->GetList();
	assert(list);

	// 疑似コード
	// for (auto [tr, render, mat] : world_.View<TransformComponent, RenderComponent, MaterialComponent>()) {
	//    GpuObjectCB obj{}; obj.gWorld = tr.world;
	//    auto objCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(obj);
	//    list->SetGraphicsRootDescriptorTable(1, objCbv.gpuHandle);
	//
	//    GpuMaterialCB mcb{};
	//    mcb.gBaseColor = mat.baseColor; ...
	//    auto matCbv = resourceSys_->GetFrameCBManager()->UploadCBAndCreateView(mcb);
	//    list->SetGraphicsRootDescriptorTable(2, matCbv.gpuHandle);
	//
	//    auto albedoSrv = textureMgr->GetSRV(mat.albedoKey); // TextureManager管轄
	//    list->SetGraphicsRootDescriptorTable(3, albedoSrv);
	//
	//    BindMeshIA(render.mesh);
	//    list->DrawIndexedInstanced(...);
	// }

	// 今は差し替えポイントとして固定
}