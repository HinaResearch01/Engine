#pragma once

#include "DX12/DX12Manager.h"
#include "Resource/Tex/TextureManager.h"
#include "Resource/Mesh/MeshManager.h"
#include "Math/TMath.h"

#include <cstdint>

namespace Tsumi::Render {

using MeshHandle = std::string;
using TextureHandle = std::string;

// 描画レイヤー
enum class RenderLayer : uint8_t 
{
	Opaque,
	Transparent,
};
// 描画アイテム情報
struct RenderItem 
{
	MeshHandle mesh;
	TextureHandle albedo;
	RenderLayer layer;
	Math::Mat4x4 world;
};

/* 描画管理クラス */
class RenderSystem {

private: // シングルトン
	RenderSystem();
	~RenderSystem() = default;
	RenderSystem(const RenderSystem&) = delete;
	const RenderSystem& operator=(const RenderSystem&) = delete;

public:
	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static RenderSystem* GetInstance() {
		static RenderSystem instance;
		return &instance;
	}

	/// <summary>
	/// 
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// 
	/// </summary>
	void EndFrame();

	/// <summary>
	/// 
	/// </summary>
	void Submit(const RenderItem& item);

	/// <summary>
	/// 
	/// </summary>
	void Render();

#pragma region Accessor

#pragma	endregion

private:
	/// <summary>
	/// Itemのソート
	/// </summary>
	void SortItems();

	/// <summary>
	/// Itemの描画
	/// </summary>
	void DrawItem(ID3D12GraphicsCommandList* list, const RenderItem& item);

private:

	std::vector<RenderItem> items_;
	
	DX12::DX12Manager* dx12Mgr_ = nullptr;
	Resource::MeshManager* meshMgr_ = nullptr;
	Resource::TextureManager* texMgr_ = nullptr;
};

}