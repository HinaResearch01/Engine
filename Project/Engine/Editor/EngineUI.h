#pragma once

#include "Utils/FixFPS/FixFPS.h"

namespace Tsumi::Framework {
class GameContext;
class IActor;
}

namespace tme::editor {

class EngineUI {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	EngineUI() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~EngineUI() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	void Init(util::FixFPS* fixFPS);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

#pragma region Accessor
	void SetGameContext(Tsumi::Framework::GameContext* context) { gameContext_ = context; }
#pragma endregion

private:
	void DrawPerformance();
	void DrawScene();
	void DrawInspector();
	void DrawResources();
	void DrawShadowDebug();
	void DrawCameraControl();

private:
	util::FixFPS* fixFPS_ = nullptr;
	Tsumi::Framework::GameContext* gameContext_ = nullptr;
	Tsumi::Framework::IActor* selectedActor_ = nullptr;
};

}
