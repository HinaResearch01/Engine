#pragma once

#include <d3d12.h>
#include <vector>
#include <cstdint>

namespace Tsumi::DX12 {

// 前方宣言
class CommandContext;

/*  */
class FrameBindState {
	
public:
	/// コンストラクタ
	/// </summary>
	FrameBindState() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameBindState() = default;

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void Begin(CommandContext& cmd);

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

	/// <summary>
	/// 
	/// </summary>
	void SetCBV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS va);

	/// <summary>
	/// 
	/// </summary>
	void SetTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE table);

private:
	std::vector<D3D12_GPU_VIRTUAL_ADDRESS> cbv_;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> table_;

	CommandContext* cmd_ = nullptr;
};

}