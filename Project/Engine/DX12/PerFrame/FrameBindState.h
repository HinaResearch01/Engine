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
	void Begin(ID3D12GraphicsCommandList* cmdList);

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

	/// <summary>
	/// Root Parameter 設定
	/// </summary>
	void SetCBV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS va);
	void SetTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle);

private:
	// 状態キャッシュ (RootIndexごとの現在の値)
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> cachedTables_;
	std::vector<D3D12_GPU_VIRTUAL_ADDRESS> cachedCBVs_;

	ID3D12GraphicsCommandList* cmdList_ = nullptr;
};

}