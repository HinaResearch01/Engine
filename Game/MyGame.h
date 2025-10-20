#pragma once

#include "../Engine/Core/GameApp.h"

namespace  Tsumi {

/* ゲーム全体のルート制御 */
class MyGame : public GameApp {
public:
    void OnInit() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnFinalize() override;
};

}