#pragma once

#include <d3d12.h>
#include <vector>
#include <cstdint>

namespace Tsumi::DX12 {

// 前方宣言
class CommandContext;

/*「このフレーム中の bind 状態」をキャッシュして重複 Set を防ぐ  */
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
	void Begin(DX12::CommandContext& cmd);

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
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> tables_;
	std::vector<D3D12_GPU_VIRTUAL_ADDRESS> cbvs_;

	CommandContext* cmd_ = nullptr;
};

}