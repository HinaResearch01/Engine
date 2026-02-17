#pragma once

#include <chrono>

namespace Tsumi::Utils {

class FixFPS {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FixFPS() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FixFPS();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// フレーム制御
	/// </summary>
	void Update();

	/// <summary>
	/// FPSの取得
	/// </summary>
	float GetFPS() const { return fps_; }

private:
	// 記録時間（FPS固定用）
	std::chrono::steady_clock::time_point reference_;
	float fps_ = 0.0f;
};

}
