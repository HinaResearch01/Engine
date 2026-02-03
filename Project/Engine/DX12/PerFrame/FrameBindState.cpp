#include "FrameBindState.h"
#include "DX12/cmd/CommandContext.h"

using namespace Tsumi::DX12;

void FrameBindState::Begin(ID3D12GraphicsCommandList* cmdList)
{
	cmdList_ = cmdList;
	cachedTables_.clear();
	cachedCBVs_.clear();
}

void FrameBindState::Reset()
{
	cmdList_ = nullptr;
	cachedTables_.clear();
	cachedCBVs_.clear();
}

void FrameBindState::SetCBV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS va)
{
	if (!cmdList_) return;

	// 配列拡張
	if (rootIndex >= cachedCBVs_.size()) {
		cachedCBVs_.resize(rootIndex + 1, 0);
	}

	// 重複チェック
	if (cachedCBVs_[rootIndex] == va) return;

	// コマンド発行 & 更新
	cmdList_->SetGraphicsRootConstantBufferView(rootIndex, va);
	cachedCBVs_[rootIndex] = va;
}

void FrameBindState::SetTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	if (!cmdList_) return;

	if (rootIndex >= cachedTables_.size()) {
		cachedTables_.resize(rootIndex + 1, { 0 });
	}

	if (cachedTables_[rootIndex].ptr == handle.ptr) return;

	cmdList_->SetGraphicsRootDescriptorTable(rootIndex, handle);
	cachedTables_[rootIndex] = handle;
}
