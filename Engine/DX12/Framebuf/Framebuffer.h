#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

namespace Tsumi::DX12 {

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

	DX12Manager* dx12Mgr_ = nullptr;
};

}

