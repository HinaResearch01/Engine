#pragma once

namespace Tsumi {

/* ゲームの基本インターフェイス（継承用） */
class GameApp {

public:

    /// <summary>
    /// 仮想デストラクタ
    /// </summary>
    virtual ~GameApp() = default;

    /// <summary>
    /// 初期化処理
    /// </summary>
    virtual void OnInit() {}

    /// <summary>
    /// 更新処理
    /// </summary>
    virtual void OnUpdate() {}

    /// <summary>
    /// 描画処理
    /// </summary>
    virtual void OnRender() {}

    /// <summary>
    /// 解放処理
    /// </summary>
    virtual void OnFinalize() {}
};

}