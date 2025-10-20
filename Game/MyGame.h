#pragma once

#include "../Engine/Core/GameApp.h"
#include "../Engine/Scene/SceneManager.h"

namespace  Tsumi {

/* ゲーム全体のルート制御 */
class MyGame : public GameApp {

public:

    /// <summary>
    /// コンストラクタ
    /// </summary>
    MyGame();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~MyGame() = default;

    /// <summary>
    /// 初期化処理
    /// </summary>
    void OnInit() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void OnUpdate() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    void OnRender() override;
    
    /// <summary>
    /// 解放処理
    /// </summary>
    void OnFinalize() override;

private:
    std::unique_ptr<SceneManager> sceneMgr_;
};

}