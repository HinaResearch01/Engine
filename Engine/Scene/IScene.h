#pragma once

namespace Tsumi {

// 前方宣言
class SceneManager;

/* シーンの基底クラス */
class IScene {

public:

	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~IScene() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Init() {};

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update() {};

	/// <summary>
	/// 背景スプライトの描画処理
	/// </summary>
	virtual void RenderBackSprite() {};

	/// <summary>
	/// ３Dオブジェクトの描画処理
	/// </summary>
	virtual void RenderModel() {};

	/// <summary>
	/// 前景スプライトの描画処理
	/// </summary>
	virtual void RenderFrontSprite() {};

	/// <summary>
	/// 解放処理
	/// </summary>
	virtual void Finalize() {};

	/// <summary>
	/// 親マネージャーのポインタ
	/// </summary>
	/// <param name="setManager"></param>
	void SetManager(SceneManager* setManager) { sceneMgr_ = setManager; }

protected:
	SceneManager* sceneMgr_ = nullptr;
};

}