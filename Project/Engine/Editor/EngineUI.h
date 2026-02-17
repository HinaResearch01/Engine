#pragma once

#include "Utils/FixFPS/FixFPS.h"

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

private:
	void DrawPerformance();
	void DrawScene();
	void DrawResources();

private:
	Utils::FixFPS* fixFPS_ = nullptr;
};

}
