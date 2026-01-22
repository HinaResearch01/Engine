#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

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
	/// GBuffer RTV
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferRtv(UINT index) const {
		if (!gbufferRtvHeap_) return {};
		if (index >= GBUFFER_COUNT) return {};
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			gbufferRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			index, gbufferRtvDescriptorSize_);
	}

	/// <summary>
	/// GBuffer DSV
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferDsv() const {
		if (!gbufferDsvHeap_) return {};
		return gbufferDsvHeap_->GetCPUDescriptorHandleForHeapStart();
	}

	/// <summary>
	/// GBuffer SRV (連続テーブル先頭)
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGBufferSrvTable() const {
		if (!gbufferSrvHeap_) return {};
		return gbufferSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	}

	/// <summary>
	/// CreateCommittedResource の ClearValue と完全一致する値でクリアする
	/// </summary>
	static const FLOAT* GetGBufferClearColor(GBufferType type) {
		switch (type) {
			case GBufferType::Albedo: {
				static const FLOAT c[4] = { 0,0,0,1 };
				return c;
			}
			case GBufferType::Normal: {
				static const FLOAT c[4] = { 0,0,1,1 }; // A=1 推奨
				return c;
			}
			case GBufferType::Material: {
				static const FLOAT c[4] = { 1,0,1,1 };
				return c;
			}
			default: {
				static const FLOAT c[4] = { 0,0,0,1 };
				return c;
			}
		}
	}

	/// <summary>
	/// Clear
	/// </summary>
	void ClearGBufferRT(ID3D12GraphicsCommandList* cmdList, UINT index, const FLOAT clearColor[4]) const {
		if (!cmdList) return;
		if (!gbufferRtvHeap_) return;
		if (gbufferRtvDescriptorSize_ == 0) return;
		if (index >= GBUFFER_COUNT) return;

		auto rtv = GetGBufferRtv(index);
		if (rtv.ptr == 0) return;

		cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}
	void ClearGBufferDepth(ID3D12GraphicsCommandList* cmdList) const {
		if (!cmdList) return;
		auto dsv = GetGBufferDsv();
		if (dsv.ptr == 0) return;
		cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
	void ClearGBuffer(ID3D12GraphicsCommandList* cmdList) const {
		ClearGBufferRT(cmdList, 0, GetGBufferClearColor(GBufferType::Albedo));
		ClearGBufferRT(cmdList, 1, GetGBufferClearColor(GBufferType::Normal));
		ClearGBufferRT(cmdList, 2, GetGBufferClearColor(GBufferType::Material));
		ClearGBufferDepth(cmdList);
	}

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
	// Heaps
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	UINT rtvDescriptorSize_ = 0;
	UINT dsvDescriptorSize_ = 0;

	// Back buffers and depth
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers_;
	std::vector<D3D12_RESOURCE_STATES> backBufferStates_;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil_;

	// Formats / size
	DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT depthStencilFormat_ = DXGI_FORMAT_D32_FLOAT;
	UINT width_ = 0;
	UINT height_ = 0;

	// Resources
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> gbufferRTs_;
	Microsoft::WRL::ComPtr<ID3D12Resource> gbufferDepth_;

	// Formats
	DXGI_FORMAT gbufferFormats_[GBUFFER_COUNT] = {
		DXGI_FORMAT_R8G8B8A8_UNORM,      // Albedo
		DXGI_FORMAT_R16G16B16A16_FLOAT, // Normal
		DXGI_FORMAT_R8G8B8A8_UNORM       // Material
	};

	// Descriptor Heaps (RTV / SRV / DSV)
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> gbufferRtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> gbufferSrvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> gbufferDsvHeap_;

	UINT gbufferRtvDescriptorSize_ = 0;
	UINT gbufferSrvDescriptorSize_ = 0;
	UINT gbufferDsvDescriptorSize_ = 0;

	DX12Manager* dx12Mgr_ = nullptr;
};

}

