#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DescriptorUtils.h"


namespace Tsumi::DX12 {

class DescriptorHeap {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	DescriptorHeap() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~DescriptorHeap() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(ID3D12Device* device, uint32_t numDescriptors, bool shaderVisible);

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize();

#pragma region Accessor
	ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }
	D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return type_; }
	uint32_t GetDescriptorSize() const { return inc_; }
	uint32_t GetCapacity() const { return capacity_; }
	UINT GetDescriptorSize() const { return descriptorSize_; }
	// index -> CPU/GPU handle
	DescriptorHandle At(uint32_t index) const;
	// CPU handle only (for Create*View dest)
	D3D12_CPU_DESCRIPTOR_HANDLE CpuAt(uint32_t index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuAt(uint32_t index) const;
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
	D3D12_DESCRIPTOR_HEAP_TYPE type_ = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uint32_t inc_ = 0;
	uint32_t capacity_ = 0;
	bool shaderVisible_ = false;
	UINT descriptorSize_ = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuBase_{ 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE gpuBase_{ 0 };
};

}