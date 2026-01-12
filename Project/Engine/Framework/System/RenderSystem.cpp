#include "RenderSystem.h"
#include "Framework/World/World.h"
#include "DX12/DX12Manager.h"
#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"

using namespace Tsumi::Framework;

RenderSystem::RenderSystem(World& world)
	: world_(world)
{
	renderQueue_.reserve(1024);
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
	resourceSys_ = Resource::ResourceSystem::GetInstance();
	psoLib_ = Graphic::PSOLibrary::GetInstance();
	rootSigLib_ = Graphic::RootSignatureLibrary::GetInstance();
}

void RenderSystem::Update(float)
{
	//// 1. キューをリセット
	//renderQueue_.clear();

	//// 2. リソース管理システムの取得
	//auto* meshMgr = resourceSys_->GetMeshManager();

	//// 3. カメラ情報の取得 (ソート用深度計算のため)
	////    CameraSystemから現在のカメラ位置を取得
	//Math::Vec3f cameraPos = { 0,0,0 };
	//if (auto* camSys = world_.GetSystem<CameraSystem>()) {
	//	cameraPos = camSys->GetCameraContext().position;
	//}

	//// 4. RenderComponentを持つ全Actorを走査
	//auto& view = world_.GetRenderCompView();

	//for (auto* actor : view.GetActors()) {
	//	auto* renderComp = actor->GetComponent<RenderComponent>();
	//	auto* transComp = actor->GetTransform();
	//	auto* matComp = actor->GetComponent<RenderComponent>();

	//	// 非表示ならスキップ
	//	if (!renderComp->isVisible) continue;

	//	// -----------------------------------------------------------
	//	// 【重要】Mesh Resolve (文字列キー -> ポインタ変換)
	//	// -----------------------------------------------------------
	//	if (renderComp->IsDirty()) {
	//		// MeshManagerに問い合わせて解決
	//		// ロードされていなければ nullptr が返ってくる
	//		auto* asset = meshMgr->GetMesh(renderComp->GetMeshKey());
	//		renderComp->ResolveMesh(asset);
	//	}

	//	auto* meshAsset = renderComp->GetResolvedMesh();
	//	if (!meshAsset) {
	//		// まだロードされていない、またはキー指定ミスのため描画不可
	//		continue;
	//	}

	//	// -----------------------------------------------------------
	//	// Material Resolve (同様にマテリアルも解決)
	//	// -----------------------------------------------------------
	//	RenderMaterial* matAsset = nullptr;
	//	if (matComp) {
	//		// if (matComp->IsDirty()) { ... } // マテリアル実装時に追加
	//		matAsset = matComp->GetResolvedMaterial();
	//	}

	//	// -----------------------------------------------------------
	//	// RenderItem 構築
	//	// -----------------------------------------------------------
	//	RenderItem item;
	//	item.mesh = meshAsset;       // 生ポインタ
	//	item.material = matAsset;    // 生ポインタ
	//	item.worldMatrix = transComp->world;
	//	item.color = renderComp->baseColor;

	//	// ソートキー計算
	//	RenderSortKey key;
	//	key.value = 0;
	//	key.fields.layer = static_cast<uint64_t>(renderComp->layer);

	//	// マテリアルID (バッチング用)
	//	if (matAsset) {
	//		key.fields.material = matAsset->GetID();
	//	}

	//	// 深度計算 (不透明は手前優先、半透明は奥優先)
	//	// 簡易的に「不透明オブジェクト」として距離計算
	//	float distSq = (transComp->GetWorldPos() - cameraPos).Length();

	//	// ソートキーは昇順(小さい方が先)なので、
	//	// 手前を描画したい場合(Opaque)は値を小さくしたい -> そのままでOK? 
	//	// 通常Opaqueはステート順優先だが、ここでは簡易化
	//	key.fields.depth = static_cast<uint32_t>(distSq * 10.0f);

	//	item.sortKey = key.value;

	//	renderQueue_.push_back(item);
	//}

	//// 5. ソート実行 (RenderItemのoperator< を使用)
	//// RenderSortKeyの定義に従って並び替え
	//std::sort(renderQueue_.begin(), renderQueue_.end(),
	//		  [](const RenderItem& a, const RenderItem& b) {
	//	return a.sortKey < b.sortKey;
	//});
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

void RenderSystem::BindSceneGlobalCB(DX12::CommandContext& cmd)
{
	cmd;
}
