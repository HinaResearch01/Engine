#pragma once

#include "../Vecor/Vector.h"
#include "../Matrix/Matrix.h"
#include "../Constants/Constants.h"

namespace Tsumi::Math::EaseFunc {

inline float InSine(float num)
{
	return 1.0f - cosf((num * Const::PI) / 2.0f);
}
inline float OutSine(float num)
{
	return sinf((num * Const::PI) / 2.0f);
}
inline float InOutSine(float num)
{
	return -(cosf(Const::PI * num) - 1.0f) / 2.0f;
}
inline float InQuad(float num)
{
	return num * num;
}
inline float OutQuad(float num)
{
	return 1.0f - (1.0f - num) * (1.0f - num);
}
inline float InOutQuad(float num)
{
	return num < 0.5f ? 2.0f * num * num : 1.0f - powf(-2.0f * num + 2.0f, 2.0f) / 2.0f;
}
inline float InCubic(float num)
{
	return num * num * num;
}
inline float OutCubic(float num)
{
	return 1.0f - powf(1.0f - num, 3.0f);
}
inline float InOutCubic(float num)
{
	return num < 0.5f ? 4.0f * num * num * num : 1.0f - powf(-2.0f * num + 2.0f, 3.0f) / 2.0f;
}
inline float InQuart(float num)
{
	return num * num * num * num;
}
inline float OutQuart(float num)
{
	return 1.0f - powf(1.0f - num, 4.0f);
}
inline float InOutQuart(float num)
{
	// fixed typo: was 20.0f -> should be 2.0f
	return num < 0.5f ? 8.0f * num * num * num * num : 1.0f - powf(-2.0f * num + 2.0f, 4.0f) / 2.0f;
}
inline float InQuint(float num)
{
	return num * num * num * num * num;
}
inline float OutQuint(float num)
{
	return 1.0f - powf(1.0f - num, 5.0f);
}
inline float InOutQuint(float num)
{
	// fixed typo: return value should produce symmetric easing
	return num < 0.5f ? 16.0f * num * num * num * num * num : 1.0f - powf(-2.0f * num + 2.0f, 5.0f) / 2.0f;
}
inline float InExpo(float num)
{
	return num == 0.0f ? 0.0f : powf(2.0f, 10.0f * num - 10.0f);
}
inline float OutExpo(float num)
{
	return num == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * num);
}
inline float InOutExpo(float num)
{
	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: num < 0.5f ? powf(2.0f, 20.0f * num - 10.0f) / 2.0f
		: (2.0f - powf(2.0f, -20.0f * num + 10.0f)) / 2.0f;
}
inline float InCirc(float num)
{
	return 1.0f - sqrtf(1.0f - (num * num));
}
inline float OutCirc(float num)
{
	return sqrtf(1.0f - powf(num - 1.0f, 2.0f));
}
inline float InOutCirc(float num)
{
	return num < 0.5f
		? (1.0f - sqrtf(1.0f - powf(2.0f * num, 2.0f))) / 2.0f
		: (sqrtf(1.0f - powf(-2.0f * num + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}
inline float InBack(float num, float intensity)
{
	const float c1 = intensity;
	const float c3 = c1 + 1.0f;

	return c3 * num * num * num - c1 * num * num;
}
inline float OutBack(float num)
{
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;

	return 1.0f + c3 * powf(num - 1.0f, 3.0f) + c1 * powf(num - 1.0f, 2.0f);
}
inline float InOutBack(float num)
{
	const float c1 = 1.70158f;
	const float c2 = c1 * 1.525f;

	return num < 0.5f
		? (powf(2.0f * num, 2.0f) * ((c2 + 1.0f) * 2.0f * num - c2)) / 2.0f
		: (powf(2.0f * num - 2.0f, 2.0f) * ((c2 + 1.0f) * (num * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}
inline float InElastic(float num)
{
	const float c4 = (2.0f * Const::PI) / 3.0f;

	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: -powf(2.0f, 10.0f * num - 10.0f) * sinf((num * 10.0f - 10.75f) * c4);
}
inline float OutElastic(float num)
{
	const float c4 = (2.0f * Const::PI) / 3.0f;

	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: powf(2.0f, -10.0f * num) * sinf((num * 10.0f - 0.75f) * c4) + 1.0f;
}
inline float InOutElastic(float num)
{
	const float c5 = (2.0f * Const::PI) / 4.5f;

	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: num < 0.5f
		? -(powf(2.0f, 20.0f * num - 10.f) * sinf((20.0f * num - 11.125f) * c5)) / 2.0f
		: (powf(2.0f, -20.0f * num + 10.0f) * sinf((20.0f * num - 11.125f) * c5)) / 2.0f + 1.0f;
}
inline float OutBounce(float num)
{
	const float n1 = 7.5625f;
	const float d1 = 2.75f;

	if (num < 1.0f / d1) {
		return n1 * num * num;
	}
	else if (num < 2.0f / d1) {
		num -= 1.5f / d1;
		return n1 * num * num + 0.75f;
	}
	else if (num < 2.5f / d1) {
		num -= 2.25f / d1;
		return n1 * num * num + 0.9375f;
	}
	else {
		num -= 2.625f / d1;
		return n1 * num * num + 0.984375f;
	}
}
inline float InBounce(float num)
{
	return 1.0f - OutBounce(1.0f - num);
}
inline float InOutBounce(float num)
{
	return num < 0.5f
		? (1.0f - OutBounce(1.0f - 2.0f * num)) / 2.0f
		: (1.0f + OutBounce(2.0f * num - 1.0f)) / 2.0f;
}
inline float WithPeak(float start, float peak, float end, float ratio)
{
	// 正規化された時間を計算
	const float midPoint = 0.5f;

	// 前半部分（start -> peak）で補間
	if (ratio < midPoint) {
		return start + (peak - start) * (ratio / midPoint);
	}
	// 後半部分（peak -> end）で補間
	else {
		return peak + (end - peak) * ((ratio - midPoint) / (1.0f - midPoint));
	}
}
}