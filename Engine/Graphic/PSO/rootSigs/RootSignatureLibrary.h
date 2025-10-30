#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

#include "Utils/Logger/UtilsLog.h"

// 前方宣言
namespace Tsumi::DX12 {
class DX12Manager;
}

namespace Tsumi::Graphic {

/* ルートシグネチャ管理 */
class RootSignatureLibrary {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	RootSignatureLibrary() = default;
	RootSignatureLibrary(DX12::DX12Manager* ptr);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RootSignatureLibrary() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// ルートシグネチャの登録
	/// </summary>
	void Register(const std::string& name, const D3D12_ROOT_SIGNATURE_DESC& desc,
		const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers = {});

	/// <summary>
	/// 指定名のルートシグネチャを取得（存在しない場合は nullptr を返す）
	/// </summary>
	ID3D12RootSignature* Get(const std::string& name) const;

	/// <summary>
	/// 指定名のルートシグネチャが登録されているか
	/// </summary>
	bool Has(const std::string& name) const;

#pragma region Accessor
	ID3D12RootSignature* Get(const std::string& name)
	{ return rootSigs_[name].Get();	}
#pragma endregion

private:
	/// <summary>
	/// ルートシグネチャの生成
	/// </summary>
	void CreateObject3D();

private:
	mutable std::mutex mutex_;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSigs_;

	DX12::DX12Manager* dx12Mgr_ = nullptr;
};

}
