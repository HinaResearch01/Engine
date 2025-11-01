#pragma once

#include "../Vecor/Vector.h"
#include "../Matrix/Matrix.h"
#include "../Constants/Constants.h"

namespace Tsumi::Math::EaseFunc {

float InSine(float num)
{
	return 1.0f - cosf((num * Const::PI) / 2.0f);
}
float OutSine(float num)
{
	return sinf((num * Const::PI) / 2.0f);
}
float InOutSine(float num)
{
	return -(cosf(Const::PI * num) - 1.0f) / 2.0f;
}
float InQuad(float num)
{
	return num * num;
}
float OutQuad(float num)
{
	return 1.0f - (1.0f - num) * (1.0f - num);
}
float InOutQuad(float num)
{
	return num < 0.5f ? 2.0f * num * num : 1.0f - pow(-2.0f * num + 2.0f, 2.0f) / 2.0f;
}
float InCubic(float num)
{
	return num * num * num;
}
float OutCubic(float num)
{
	return 1.0f - pow(1.0f - num, 3.0f);
}
float InOutCubic(float num)
{
	return num < 0.5f ? 4.0f * num * num * num : 1.0f - pow(-2.0f * num + 2.0f, 3.0f) / 2.0f;
}
float InQuart(float num)
{
	return num * num * num * num;
}
float OutQuart(float num)
{
	return 1.0f - pow(1.0f - num, 4.0f);
}
float InOutQuart(float num)
{
	return num < 0.5f ? 8.0f * num * num * num * num : 1.0f - pow(-2.0f * num + 20.0f, 4.0f) / 2.0f;
}
float InQuint(float num)
{
	return num * num * num * num * num;
}
float OutQuint(float num)
{
	return 1.0f - pow(1.0f - num, 5.0f);
}
float InOutQuint(float num)
{
	return num < 0.5f ? 16.0f * num * num * num * num * num : .0f - pow(-2.0f * num + 2.0f, 5.0f);
}
float InExpo(float num)
{
	return num == 0.0f ? 0.0f : pow(2.0f, 10.0f * num - 10.0f);
}
float OutExpo(float num)
{
	return num == 1.0f ? 1.0f : 1.0f - pow(2.0f, -10.0f * num);
}
float InOutExpo(float num)
{
	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: num < 0.5f ? pow(2.0f, 20.0f * num - 10.0f) / 2.0f
		: (2.0f - pow(2.0f, -20.0f * num + 10.0f)) / 2.0f;
}
float InCirc(float num)
{
	return 1.0f - sqrt(1.0f - pow(num, 2.0f));
}
float OutCirc(float num)
{
	return sqrt(1.0f - pow(num - 1.0f, 2.0f));
}
float InOutCirc(float num)
{
	return num < 0.5f
		? (1.0f - sqrt(1.0f - pow(2.0f * num, 2.0f))) / 2.0f
		: (sqrt(1.0f - pow(-2.0f * num + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}
float InBack(float num, float intensity)
{
	const float c1 = intensity;
	const float c3 = c1 + 1.0f;

	return c3 * num * num * num - c1 * num * num;
}
float OutBack(float num)
{
	const float c1 = 1.70158f;
	const float c3 = c1 + 1;

	return 1.0f + c3 * pow(num - 1.0f, 3.0f) + c1 * pow(num - 1.0f, 2.0f);
}
float InOutBack(float num)
{
	const float c1 = 1.70158f;
	const float c2 = c1 * 1.525f;

	return num < 0.5f
		? (pow(2.0f * num, 2.0f) * ((c2 + 1.0f) * 2.0f * num - c2)) / 2.0f
		: (pow(2.0f * num - 2.0f, 2.0f) * ((c2 + 1.0f) * (num * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}
float InElastic(float num)
{
	const float c4 = (2.0f * Const::PI) / 3.0f;

	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: -pow(2.0f, 10.0f * num - 10.0f) * sinf((num * 10.0f - 10.75f) * c4);
}
float OutElastic(float num)
{
	const float c4 = (2.0f * Const::PI) / 3.0f;

	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: pow(2.0f, -10.0f * num) * sinf((num * 10.0f - 0.75f) * c4) + 1.0f;
}
float InOutElastic(float num)
{
	const float c5 = (2.0f * Const::PI) / 4.5f;

	return num == 0.0f
		? 0.0f
		: num == 1.0f
		? 1.0f
		: num < 0.5f
		? -(pow(2.0f, 20.0f * num - 10.f) * sinf((20.0f * num - 11.125f) * c5)) / 2.0f
		: (pow(2.0f, -20.0f * num + 10.0f) * sinf((20.0f * num - 11.125f) * c5)) / 2.0f + 1.0f;
}
float InBounce(float num)
{
	return 1.0f - OutBounce(1.0f - num);
}
float OutBounce(float num)
{
	const float n1 = 7.5625f;
	const float d1 = 2.75f;

	if (num < 1.0f / d1) {
		return n1 * num * num;
	}
	else if (num < 2.0f / d1) {
		return n1 * (num -= 1.5f / d1) * num + 0.75f;
	}
	else if (num < 2.5f / d1) {
		return n1 * (num -= 2.25f / d1) * num + 0.9375f;
	}
	else {
		return n1 * (num -= 2.625f / d1) * num + 0.984375f;
	}
}
float InOutBounce(float num)
{
	return num < 0.5f
		? (1.0f - OutBounce(1.0f - 2.0f * num)) / 2.0f
		: (1.0f + OutBounce(2.0f * num - 1.0f)) / 2.0f;
}
float WithPeak(float start, float peak, float end, float ratio)
{
	// 正規化された時間を計算
	float midPoint = 0.5f;

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