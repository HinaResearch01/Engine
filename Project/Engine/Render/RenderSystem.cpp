#include "RenderSystem.h"
#include "Utils/Logger/UtilsLog.h"

using namespace Tsumi::Render;

RenderSystem::RenderSystem()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
	meshMgr_ = Resource::MeshManager::GetInstance();
	texMgr_ = Resource::TextureManager::GetInstance();
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
	// Item ソート
	SortItems();

	auto* cmdList = dx12Mgr_->GetCmdList();
	if (!cmdList) return;

	// アイテム描画
	for (auto& item : items_)
	{
		DrawItem(cmdList, item);
	}
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

void RenderSystem::DrawItem(ID3D12GraphicsCommandList* list, const RenderItem& item)
{
	list, item;
}
