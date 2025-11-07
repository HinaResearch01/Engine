#pragma once

#include "../Vector/Vector.h"
#include "../Matrix/Matrix.h"
#include <iostream>
#include <cstdint>
#include <string>
#include <cassert>
#include <wrl.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <algorithm>
#include <numbers>
#include <type_traits>
#include <vector>

namespace Tsumi::Math::Func {

namespace NUM {
// ラープ
float Lerp(const float& start, const float& end, float t);
// クランプ
float Clamp(const float& value, const float& minValue, const float& maxValue);
// 0に近づくほど1になり、1や-1になるほど0を返す関数
float APOneAsZeroCloser(float value);
// 角度を度からラジアンに変換する処理
float ToRadians(float degrees);
// 範囲に変換
float ConvertToRange(Vec2f input, Vec2f output, float value);
}


namespace VEC2 {
// 内積
float Dot(const Vec2f& v1, const Vec2f& v2);
// 外積
float Cross(const Vec2f& v1, const Vec2f& v2);
// 線形補間
Vec2f Lerp(const Vec2f& a, const Vec2f& b, float t);
// 距離
float Distance(const Vec2f& v1, const Vec2f& v2);
// 絶対値
Vec2f Absolute(const Vec2f& v);
// 正射影ベクトル
Vec2f Project(const Vec2f& v1, const Vec2f& v2);
}


namespace VEC3 {
// 内積
float Dot(const Vec3f& v1, const Vec3f& v2);
// 外積
Vec3f Cross(const Vec3f& v1, const Vec3f& v2);
// 距離
float Distance(const Vec3f& a, const Vec3f& b);
// 絶対値
Vec3f Absolute(const Vec3f& v);
// 正射影ベクトル
Vec3f Project(const Vec3f& v1, const Vec3f& v2);
// 線形補間
Vec3f Lerp(const Vec3f& start, const Vec3f& end, const float t);
// 球面線形補間
Vec3f SLerp(const Vec3f& start, const Vec3f& end, const float t);
// 法線ベクトル
Vec3f Perpendicular(const Vec3f& v);
// 座標変換
Vec3f TransformByMatrix(const Vec3f v, const Mat4x4 m);
// Y軸周りに回転させる関数
Vec3f YawRotation(const Vec3f& vec, float angle);
// ベクトル変換
Vec3f TransformNormal(const Vec3f& vec, const Vec3f& rotation);
Vec3f TransformNormal(const Vec3f& v, const Mat4x4& m);
// Vec2fをそのままVec3fに入れる
Vec3f Vec3fFromVec2f(const Vec2f& v);
// CatmullRom補間
Vec3f CatmullRomInterpolation(const Vec3f& p0, const Vec3f& p1, const Vec3f& p2, const Vec3f& p3, float t);
// CatmullRomスプライン曲線上の座標を得る
Vec3f CatmullRomPosition(const std::vector<Vec3f>& points, uint32_t index, float t);
// Vec3fにアフィン変換と透視補正を適用する
Vec3f TransformWithPerspective(const Vec3f& v, const Mat4x4& m);
// 角度を 0～2π の範囲に正規化
float NormalizeAngle(float angle);
// 最短回転角度を求める
float ShortestAngle(float currentAngle, float targetAngle);
}


namespace VEC4 {
float Dot(const Vec4f& a, const Vec4f& b);
float Distance(const Vec4f& a, const Vec4f& b);
}


namespace MAT3x3 {
}


namespace MAT4x4{
// 3次元アフィン変換行列 (W = SRT)
Mat4x4 AffineMatrix(const Vec3f& scale, const Vec3f& rotate, const Vec3f& translate);
// 透視投影行列
Mat4x4 PerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
// 正射影行列
Mat4x4 OrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
// ビューポート変換行列
Mat4x4 ViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
// 任意軸回転行列
Mat4x4 RotateAxisAngle(const Vec3f& axis, float angle);
Mat4x4 RotateAxisAngle(const Vec3f& axis, float cos, float sin);
// ある方向からある方向への回転
Mat4x4 DirectionToDirection(const Vec3f& from, const Vec3f& to);
}


namespace Interpolation {
/// <summary>
/// 汎用的な補間処理を行う関数。
/// イージング関数を指定して、開始値から終了値へのスムーズな補間を実現します。
/// </summary>
/// <typeparam name="T">補間する値の型（例: float, Vec3f など）</typeparam>
/// <param name="startValue">開始値</param>
/// <param name="endValue">終了値</param>
/// <param name="ratio">進行度（0.0f～1.0fの範囲）</param>
/// <param name="easingFunc">イージング関数（例: Ease::OutExpo）</param>
/// <returns>補間された値</returns>
template <typename T, typename Func>
T Interpolate(const T& startValue, const T& endValue, float ratio, Func easingFunc) {
    float easedRatio = easingFunc(ratio); // イージング関数で進行度を補正
    return startValue + (endValue - startValue) * easedRatio;
}
/// <summary>
/// 汎用的な補間処理を行う関数。
/// イージング関数を指定して、開始値から終了値へのスムーズな補間を実現します。
/// </summary>
/// <typeparam name="T">補間する値の型（例: float, Vec3f など）</typeparam>
/// <param name="start">開始</param>
/// <param name="peak">ピーク値</param>
/// <param name="end">開始&終了値</param>
/// <param name="ratio">進行度（0.0f～1.0fの範囲）</param>
/// <returns>補間された値</returns>
template <typename T, typename Func>
T InterpolateWithPeak(const T& start, const T& peak, const T& end, float ratio, Func easingFunc) {
    T result = easingFunc(start, peak, end, ratio);
    return result;
}
}
}