#pragma once

#include <string>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>

namespace Tsumi::Resource {

/* 静的リソース＋SRV管理 */
class StaticResourceManager {

private: // シングルトン
	StaticResourceManager();
	~StaticResourceManager();
	~StaticResourceManager(const StaticResourceManager&) = delete;
	const StaticResourceManager& operator=(const StaticResourceManager&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static StaticResourceManager* GetInstance() {
		static StaticResourceManager instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();



private:

private:

};

}