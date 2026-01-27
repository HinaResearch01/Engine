#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cassert>
#include "DescriptorHandles.h"

namespace Tsumi::DX12 {

// 前方宣言
class DX12Manager;

/* 
SRV / CBV / UAV の 登録専用
GPU handl は返さない。CPU-OnlyのHeap
*/
class PersistentDescriptorAllocator {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	PersistentDescriptorAllocator() = default;
	PersistentDescriptorAllocator(DX12Manager* ptr);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~PersistentDescriptorAllocator() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity);

	/// <summary>
	/// 割り当て
	/// </summary>
	CpuDescHandle Allocate();

#pragma region Accessor
	ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
	uint32_t capacity_ = 0;
	uint32_t used_ = 0;
	uint32_t incSize_ = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuStart_{ 0 };

	DX12Manager* dx12Mgr_ = nullptr;
};

}