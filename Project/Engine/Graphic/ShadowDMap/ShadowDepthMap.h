#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

// 前方宣言
namespace Tsumi::DX12{ class DX12Manager; }

namespace Tsumi::Graphic {

/* ShadowDepthMap（Resource 所有クラス） */
class ShadowDepthMap {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	ShadowDepthMap() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~ShadowDepthMap() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init(uint32_t size);
	
	/// <summary>
	/// 再生成
	/// </summary>
	void Resize(uint32_t size);

#pragma region Accessor 
	uint32_t GetSize() const { return size_; }
	ID3D12Resource* GetResource() const { return tex_.Get(); }
#pragma endregion

	// DSV / SRV 用フォーマット規約
	static constexpr DXGI_FORMAT kDSVFormat = DXGI_FORMAT_D32_FLOAT;
	static constexpr DXGI_FORMAT kSRVFormat = DXGI_FORMAT_R32_FLOAT;

private:
	/// <summary>
	/// 生成
	/// </summary>
	void Create(uint32_t size);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> tex_;
	uint32_t size_ = 0;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}
