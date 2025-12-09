#pragma once

namespace Tsumi {

// 前方宣言
class SceneManager;
class GameContext;

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
	void SetManager(SceneManager* setManager) { sceneMgr_ = setManager; }

	/// <summary>
	/// GameContextのポインタ設定
	/// </summary>
	void SetContext(GameContext* setPtr) { gameContext_ = setPtr; }

protected:
	SceneManager* sceneMgr_ = nullptr;
	GameContext* gameContext_ = nullptr;
};

}