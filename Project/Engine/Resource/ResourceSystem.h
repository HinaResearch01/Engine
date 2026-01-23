#pragma once

#include <memory>

// 前方宣言
namespace Tsumi::DX12 { class DX12Manager; }
namespace Tsumi::Resource {
class FrameCBManager;
class MeshManager;
class GpuViewManager;
class TextureManager;
}

namespace Tsumi::Resource {

/* リソース管理クラス */
class ResourceSystem {

private: // シングルトン
	ResourceSystem();
	~ResourceSystem();
	ResourceSystem(const ResourceSystem&) = delete;
	const ResourceSystem& operator=(const ResourceSystem&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static ResourceSystem* GetInstance() {
		static ResourceSystem instance;
		return &instance;
	}
	
	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// フレーム開始処理
	/// </summary>
	void BeginFrame(uint32_t frameIndex);

	/// <summary>
	/// シーンリセット処理
	/// </summary>
	void SceneReset();

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize();

#pragma region Accessor
	FrameCBManager* GetFrameCBManager() const { return cbMgr_.get(); }
	MeshManager* GetMeshManager() const { return meshMgr_.get(); }
	GpuViewManager* GetGpuViewManager() const { return gViewMgr_.get(); }
	TextureManager* GetTextureManager() const { return texMgr_.get(); }
#pragma endregion

private:
	std::unique_ptr<FrameCBManager> cbMgr_;
	std::unique_ptr<MeshManager> meshMgr_;
	std::unique_ptr<GpuViewManager> gViewMgr_;
	std::unique_ptr<TextureManager> texMgr_;
	Tsumi::DX12::DX12Manager* dx12Mgr_ = nullptr;
};
}