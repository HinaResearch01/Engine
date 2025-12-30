#include "RenderSystem.h"
#include "Utils/Logger/UtilsLog.h"

using namespace Tsumi::Render;

RenderSystem::RenderSystem()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
	resourceSys_ = Resource::ResourceSystem::GetInstance();
	psoLib_ = Graphic::PSOLibrary::GetInstance();
	rootSigLib_ = Graphic::RootSignatureLibrary::GetInstance();
}

void RenderSystem::BeginFrame()
{
	items_.clear();
}

void RenderSystem::EndFrame()
{
}

void RenderSystem::Submit(const RenderItem& item)
{
	if (item.mesh.empty()) {
		Utils::Error(L"[RenderSystem] Add() mesh key is empty.");
		return;
	}
	items_.push_back(item);
}

void RenderSystem::Render()
{
	// コマンドリスト取得
	auto* cmdList = dx12Mgr_->GetCmdList();
	if (!cmdList) return;

	SortItems(); // Item ソート
	SetPSOAndRootSig(cmdList); // PSO と RootSig 設定

	// Item 描画
	for (auto& item : items_) {
		DrawItem(cmdList, item); 
	}
}

void RenderSystem::DrawItem(ID3D12GraphicsCommandList* list, const RenderItem& item)
{
	auto* mesh = resourceSys_->GetMeshManager()->GetMesh(item.mesh);


	resourceSys_->GetMeshManager()->GetMesh(item.mesh)->indexCount;


	// --1) 頂点・インデックスバッファ設定 --
	list->IASetVertexBuffers(0, 1, &mesh->vbView);
	list->IASetIndexBuffer(&mesh->ibView);

	// --2) 定数バッファ設定 --


	// --3) テクスチャ設定 --


	// --4) Draw --
	list->DrawIndexedInstanced(mesh->indexCount, 1, 0, 0, 0);
}

void RenderSystem::SortItems()
{
	// まずは layer -> texture -> mesh の順でソート
	std::sort(items_.begin(), items_.end(),
			  [](const RenderItem& a, const RenderItem& b)
	{
		if (a.layer != b.layer) return a.layer < b.layer;
		if (a.albedo != b.albedo) return a.albedo < b.albedo;
		return a.mesh < b.mesh;
	});
}

void RenderSystem::SetPSOAndRootSig(ID3D12GraphicsCommandList* list)
{
	/* 
	--- 要修正 ---
		現在 Object3D 固定なので、将来的にアイテム毎に切り替えるようにする
	*/
	list->SetGraphicsRootSignature(rootSigLib_->Get("Object3D"));
	list->SetPipelineState(psoLib_->Get(L"Object3D"));
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

