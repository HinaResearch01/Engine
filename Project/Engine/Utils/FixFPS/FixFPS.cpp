#include "FixFPS.h"
#include <thread>
#include <Windows.h>
#pragma comment(lib, "winmm.lib")

using namespace Tsumi::Utils;

void FixFPS::Init() {
    // タイマー精度を上げる (1ms)
    timeBeginPeriod(1);
    reference_ = std::chrono::steady_clock::now();
}

FixFPS::~FixFPS() {
    timeEndPeriod(1);
}

void FixFPS::Update() {
    // 1/60秒ぴったりの時間
    const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
    // 1/60秒よりわずかに短い時間
    const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

    // 現在時間を取得する
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    // 前回記録からの経過時間を取得する
    std::chrono::microseconds elapsed = 
        std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);



    // 1/60秒(よりわずかに短い時間)経っていない場合
    if (elapsed < kMinCheckTime) {
        // 1/60秒経過するまで微小なスリープを繰り返す
        while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

	// FPS計算 (経過時間が0の場合はスキップ)
	// 
	// sleep後の時間 = 実質的な1フレームの時間
	//
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::chrono::microseconds totalElapsed = 
		std::chrono::duration_cast<std::chrono::microseconds>(end - reference_);

	if (totalElapsed.count() > 0) {
		fps_ = 1000000.0f / static_cast<float>(totalElapsed.count());
	}

    // 現在の時間を記録する
    reference_ = std::chrono::steady_clock::now();
}
