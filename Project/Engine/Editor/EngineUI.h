#pragma once

#include "Utils/FixFPS/FixFPS.h"

namespace Tsumi::Framework {
class GameContext;
class IActor;
}

namespace Tsumi::Editor {

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
	void Init(Utils::FixFPS* fixFPS);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

#pragma region Accessor
	void SetGameContext(Framework::GameContext* context) { gameContext_ = context; }
#pragma endregion

private:
	void DrawPerformance();
	void DrawScene();
	void DrawInspector();
	void DrawResources();

private:
	Utils::FixFPS* fixFPS_ = nullptr;
	Framework::GameContext* gameContext_ = nullptr;
	Framework::IActor* selectedActor_ = nullptr;
};

}
