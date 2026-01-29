#include "FrameBindState.h"
#include "DX12/cmd/CommandContext.h"

using namespace Tsumi::DX12;

void FrameBindState::Begin(CommandContext& cmd)
{
	cmd_ = &cmd;
	Reset();
}

void FrameBindState::Reset()
{
	cbv_.clear();
	table_.clear();
}

void FrameBindState::SetCBV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS va)
{
	if (cbv_.size() <= rootIndex) {
		cbv_.resize(static_cast<std::vector<D3D12_GPU_VIRTUAL_ADDRESS, 
					std::allocator<D3D12_GPU_VIRTUAL_ADDRESS>>::size_type>
					(rootIndex) + 1, 0);
	}
	if (cbv_[rootIndex] == va) {
		return; // 変更なし
	}
	cbv_[rootIndex] = va;
	cmd_->GetList()->SetGraphicsRootConstantBufferView(rootIndex, va);
}

void FrameBindState::SetTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE table)
{
	if (table_.size() <= rootIndex) {
		table_.resize(static_cast<std::vector<D3D12_GPU_DESCRIPTOR_HANDLE, 
					  std::allocator<D3D12_GPU_DESCRIPTOR_HANDLE>>::size_type>
					  (rootIndex) + 1, { 0 });
	}
	if (table_[rootIndex].ptr == table.ptr) {
		return; // 変更なし
	}
	table_[rootIndex] = table;
	cmd_->GetList()->SetGraphicsRootDescriptorTable(rootIndex, table);
}
