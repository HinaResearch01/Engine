#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>
#include "DX12/Desc/DescriptorUtils.h"
#include "DX12/Desc/DescriptorHeap.h"

namespace Tsumi::DX12 {

// Framebuffer.h の上の方
constexpr UINT GBUFFER_COUNT = 3;

enum class GBufferType : UINT {
	Albedo = 0,
	Normal = 1,
	Material = 2,
};

// 前方宣言
class DX12Manager;

/* バックバッファ（RTV）と深度ステンシル（DSV）をまとめて管理するクラス */
class Framebuffer {

public:
	/// <summary>
	/// コンストラクタ
	/// コピー禁止
	/// </summary>
	Framebuffer(DX12Manager* ptr);
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~Framebuffer();

	/// <summary>
	/// 初期化処理
	/// </summary>
	HRESULT Init();

	/// <summary>
	/// 破棄
	/// </summary>
	void Destroy();

	/// <summary>
	/// リサイズ
	/// </summary>
	HRESULT Resize(UINT width, UINT height);

	/// <summary>
	/// RTV/DSV ハンドル取得
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(UINT index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

	/// <summary>
	/// バックバッファリソース取得
	/// </summary>
	ID3D12Resource* GetBackBuffer(UINT index) const;

	/// <summary>
	/// バッファ数
	/// </summary>
	UINT GetBufferCount() const;

	/// <summary>
	/// クリア
	/// </summary>
	void ClearRenderTarget(ID3D12GraphicsCommandList* cmdList, UINT rtvIndex, const FLOAT clearColor[4]);
	void ClearDepthStencil(ID3D12GraphicsCommandList* cmdList, FLOAT depth = 1.0f, UINT8 stencil = 0);

	/// <summary>
	/// バックバッファのリソースステート取得/設定
	/// </summary>
	D3D12_RESOURCE_STATES GetBackBufferState(UINT index) const;
	void SetBackBufferState(UINT index, D3D12_RESOURCE_STATES state);

	/// <summary>
	/// GBuffer RTV / DSV
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferRtv(UINT index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferDsv() const;

	/// <summary>
	/// CreateCommittedResource の ClearValue と完全一致する値でクリアする
	/// </summary>
	static const FLOAT* GetGBufferClearColor(GBufferType type);

	/// <summary>
	/// Clear
	/// </summary>
	void ClearGBuffer(ID3D12GraphicsCommandList* cmdList) const;

#pragma region Accessor
	UINT GetWidth() const { return width_; }
	UINT GetHeight() const { return height_; }
	UINT GetBackBufferCount() const { return static_cast<UINT>(backBuffers_.size()); }
#pragma endregion

private:
	/// <summary>
	/// HeapとViewの生成
	/// </summary>
	/// <returns></returns>
	HRESULT CreateHeapsAndViews(UINT width, UINT height);

	/// <summary>
	/// Viewの解放
	/// </summary>
	void ReleaseViews();

private:
	// SwapChain RTV / Main DSV
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	UINT rtvDescriptorSize_ = 0;
	UINT dsvDescriptorSize_ = 0;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers_;
	std::vector<D3D12_RESOURCE_STATES> backBufferStates_;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil_;

	// GBuffer resources
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> gbufferRTs_;
	Microsoft::WRL::ComPtr<ID3D12Resource> gbufferDepth_;

	// GBuffer RTV / DSV heaps
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> gbufferRtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> gbufferDsvHeap_;
	UINT gbufferRtvDescriptorSize_ = 0;

	// Persistent SRV table
	DescriptorHandle gbufferSrvBase_{};

	// formats / size
	DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT depthStencilFormat_ = DXGI_FORMAT_D32_FLOAT;
	DXGI_FORMAT gbufferFormats_[GBUFFER_COUNT] = {
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM
	};

	UINT width_ = 0;
	UINT height_ = 0;

	DX12Manager* dx12Mgr_ = nullptr;
};

}