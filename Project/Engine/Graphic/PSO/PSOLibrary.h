#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

#include "Utils/Logger/Logger.h"

// 前方宣言
namespace Tsumi::DX12 {
class DX12Manager;
}
namespace Tsumi::Graphic {
class ShaderLibrary;
class RootSignatureLibrary;
}

namespace Tsumi::Graphic {

/* PSO管理 */
class PSOLibrary {

private: // シングルトン
	PSOLibrary() = default;
	~PSOLibrary() = default;
	PSOLibrary(const PSOLibrary&) = delete;
	const PSOLibrary& operator=(const PSOLibrary&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static PSOLibrary* GetInstance() {
		static PSOLibrary instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// PipelineStateの取得
	/// </summary>
	void Register(const std::string& name, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso);

	/// <summary>
	/// PipelineStateの取得
	/// </summary>
	ID3D12PipelineState* Get(const std::string& name);

	/// <summary>
	/// 指定名のPipelienStateが登録されているか
	/// </summary>
	bool Has(const std::string& name) const;

private:
	mutable std::mutex mutex_;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineMap_;

};
}
