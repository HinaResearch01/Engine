#include "FrameBindState.h"
#include "DX12/cmd/CommandContext.h"

using namespace Tsumi::DX12;

void FrameBindState::Begin(DX12::CommandContext& cmd)
{
	cmd_ = &cmd;
	tables_.clear();
	cbvs_.clear();
}

void FrameBindState::Reset()
{
	cmd_ = nullptr;
	tables_.clear();
	cbvs_.clear();
}

void FrameBindState::SetCBV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS va)
{
	if (!cmd_) return;

	if (cbvs_.size() <= rootIndex) {
		cbvs_.resize(rootIndex + 1, 0);
	}

	if (cbvs_[rootIndex] == va) {
		return; // 重複回避
	}

	cbvs_[rootIndex] = va;
	cmd_->SetGraphicsRootConstantBufferView(rootIndex, va);
}

void FrameBindState::SetTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE table)
{
	if (!cmd_) return;

	if (tables_.size() <= rootIndex) {
		tables_.resize(rootIndex + 1, D3D12_GPU_DESCRIPTOR_HANDLE{ 0 });
	}

	if (tables_[rootIndex].ptr == table.ptr) {
		return; // 重複回避
	}

	tables_[rootIndex] = table;
	cmd_->SetGraphicsRootDescriptorTable(rootIndex, table);
}
