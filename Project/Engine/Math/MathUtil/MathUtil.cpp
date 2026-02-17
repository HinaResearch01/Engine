#include "MathUtil.h"
#include "../Constants/Constants.h"

using namespace Tsumi::Math;
using namespace Func;

float NUM::Lerp(const float& start, const float& end, float t)
{
	return start + (end - start) * t;
}

float NUM::Clamp(const float& value, const float& minValue, const float& maxValue)
{
	return std::clamp(value, minValue, maxValue);
}

float NUM::APOneAsZeroCloser(float value)
{
	return expf(-powf(value, 2.0f));
}

float NUM::ToRadians(float degrees)
{
	return degrees * (Const::PI / 180.0f);
}

float NUM::ToDegrees(float radians)
{
	return radians * (180.0f / Const::PI);
}

float NUM::ConvertToRange(const Vec2f& input, const Vec2f& output, float value)
{
	// 入力値を入力範囲に正規化
	float normalize = (value - input.x) / (input.y - input.x);

	// 正規化された値を出力範囲にスケーリング
	float result = normalize * (output.y - output.x) + output.x;

	return result;
}

float VEC2::Dot(const Vec2f& v1, const Vec2f& v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y);
}

float VEC2::Cross(const Vec2f& v1, const Vec2f& v2)
{
	return (v1.x * v2.y) - (v1.y * v2.x);
}

Vec2f VEC2::Lerp(const Vec2f& a, const Vec2f& b, float t)
{
	return a + (b - a) * t;
}

float VEC2::Distance(const Vec2f& v1, const Vec2f& v2)
{
	return (v1 - v2).Length();
}

Vec2f VEC2::Absolute(const Vec2f& v)
{
	return { std::abs(v.x), std::abs(v.y) };
}

Vec2f VEC2::Project(const Vec2f& v1, const Vec2f& v2)
{
	Vec2f n = v2.Normalize();
	return Dot(v1, n) * n;
}

float VEC3::Dot(const Vec3f& v1, const Vec3f& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vec3f VEC3::Cross(const Vec3f& v1, const Vec3f& v2)
{
	return {
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x
	};
}

float VEC3::Distance(const Vec3f& v1, const Vec3f& v2)
{
	return (v1 - v2).Length();
}

Vec3f VEC3::Absolute(const Vec3f& v)
{
	return { std::abs(v.x), std::abs(v.y), std::abs(v.z) };
}

Vec3f VEC3::Project(const Vec3f& v1, const Vec3f& v2)
{
	Vec3f n = v2.Normalized();
	return Dot(v1, n) * n;
}

Vec3f VEC3::Lerp(const Vec3f& start, const Vec3f& end, const float t)
{
	return start + t * (end - start);
}

Vec3f VEC3::SLerp(const Vec3f& start, const Vec3f& end, const float t)
{
	Vec3f v0 = start.Normalized();
	Vec3f v1 = end.Normalized();

	float dot = VEC3::Dot(v0, v1);
	dot = std::clamp(dot, -1.0f, 1.0f);

	float theta = std::acos(dot) * t;

	Vec3f relative = (v1 - v0 * dot).Normalized();

	return v0 * std::cos(theta) + relative * std::sin(theta);
}

Vec3f VEC3::Perpendicular(const Vec3f& v)
{
	if (v.x != 0.0f || v.y != 0.0f) {
		return { -v.y, v.x, 0.0f };
	}
	return { 0.0f, -v.z, v.y };
}

Vec3f VEC3::TransformByMatrix(const Vec3f v, const Mat4x4 m)
{
	Vec3f result{};

	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z *
		m.m[2][0] + 1.0f * m.m[3][0];

	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z *
		m.m[2][1] + 1.0f * m.m[3][1];

	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z *
		m.m[2][2] + 1.0f * m.m[3][2];

	float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z *
		m.m[2][3] + 1.0f * m.m[3][3];

	assert(w != 0.0f);

	result.x /= w;
	result.y /= w;
	result.z /= w;

	return  result;
}

Vec3f VEC3::YawRotation(const Vec3f& vec, float angle)
{
	// Y軸回転行列を適用
	float cosYaw = std::cos(angle);
	float sinYaw = std::sin(angle);

	return {
		vec.x * cosYaw - vec.z * sinYaw,
		vec.y,
		vec.x * sinYaw + vec.z * cosYaw
	};
}

Vec3f VEC3::TransformNormal(const Vec3f& vec, const Vec3f& rotation)
{
	// Y軸回転行列を適用
	float cosYaw = std::cos(rotation.y);
	float sinYaw = std::sin(rotation.y);

	return {
		vec.x * cosYaw + vec.z * sinYaw,
		vec.y,
		-vec.x * sinYaw + vec.z * cosYaw
	};
}

Vec3f VEC3::TransformNormal(const Vec3f& v, const Mat4x4& m)
{
	Vec3f result{};

	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];

	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];

	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];

	return result;
}

Vec3f VEC3::Vec3fFromVec2f(const Vec2f& v)
{
	return { v.x, v.y, 0.0f };
}

Vec3f VEC3::CatmullRomInterpolation(const Vec3f& p0, const Vec3f& p1, const Vec3f& p2, const Vec3f& p3, float t)
{
	float t2 = t * t;
	float t3 = t * t * t;
	return Vec3f(
		//x
		0.5f * ((-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * t3 +
		(2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t2 + (-p0.x + p2.x) * t + 2 * p1.x),
		//y
		0.5f * ((-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * t3 +
		(2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t2 + (-p0.y + p2.y) * t + 2 * p1.y),
		//z
		0.5f * ((-p0.z + 3 * p1.z - 3 * p2.z + p3.z) * t3 +
		(2 * p0.z - 5 * p1.z + 4 * p2.z - p3.z) * t2 + (-p0.z + p2.z) * t + 2 * p1.z)
	);
}

Vec3f VEC3::CatmullRomPosition(const std::vector<Vec3f>& points, uint32_t index, float t)
{
	const uint32_t kIndex = uint32_t(points.size() - 1);

	int index0 = ((index - 1) + kIndex) % kIndex;
	int index1 = index;
	int index2 = (index + 1) % kIndex;
	int index3 = (index + 2) % kIndex;

	Vec3f p0 = points[index0];
	Vec3f p1 = points[index1];
	Vec3f p2 = points[index2];
	Vec3f p3 = points[index3];

	return CatmullRomInterpolation(p0, p1, p2, p3, t);
}

Vec3f VEC3::TransformWithPerspective(const Vec3f& v, const Mat4x4& m)
{
	Vec3f result = {
		(v.x * m.m[0][0]) + (v.y * m.m[1][0]) + (v.z * m.m[2][0]) + (1.0f * m.m[3][0]),
		(v.x * m.m[0][1]) + (v.y * m.m[1][1]) + (v.z * m.m[2][1]) + (1.0f * m.m[3][1]),
		(v.x * m.m[0][2]) + (v.y * m.m[1][2]) + (v.z * m.m[2][2]) + (1.0f * m.m[3][2])
	};
	float w = (v.x * m.m[0][3]) + (v.y * m.m[1][3]) + (v.z * m.m[2][3]) + (1.0f * m.m[3][3]);

	//0除算を避ける
	if (w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}

	return result;
}

float VEC3::NormalizeAngle(float angle)
{
	while (angle < -Math::Const::PI) angle += Math::Const::Double_PI;
	while (angle > Math::Const::PI) angle -= Math::Const::Double_PI;
	return angle;
}

float VEC3::ShortestAngle(float currentAngle, float targetAngle)
{
	// 角度の差を計算
	float angleDifference = targetAngle - currentAngle;

	// 角度を -π から +π の範囲に正規化する
	while (angleDifference > Math::Const::PI) angleDifference -= 2.0f * Math::Const::PI;
	while (angleDifference < -Math::Const::PI) angleDifference += 2.0f * Math::Const::PI;

	return angleDifference; // 最短回転角度を返す
}

Vec3f VEC3::ToRadians(const Vec3f& degree)
{
	Vec3f result = {
		NUM::ToRadians(degree.x),
		NUM::ToRadians(degree.y),
		NUM::ToRadians(degree.z),
	};
	return result;
}

float VEC4::Dot(const Vec4f& a, const Vec4f& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float VEC4::Distance(const Vec4f& a, const Vec4f& b)
{
	return (a - b).Length();
}

Mat3x3 MAT3x3::BuildUVMatrix(const UVTransform& uv)
{
	return
		Mat3x3::Scale(uv.scale) *
		Mat3x3::Rotation(uv.rotation) *
		Mat3x3::Translation(uv.translate);
}

Mat4x4 MAT4x4::AffineMatrix(const Vec3f& scale, const Vec3f& rotate, const Vec3f& translate)
{
	Mat4x4 s = Mat4x4::Scale(scale);
	Mat4x4 r = Mat4x4::Rotation(rotate);
	Mat4x4 t = Mat4x4::Translation(translate);

	return t * r * s;
}

Mat4x4 MAT4x4::PerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
	Mat4x4 result{}; 

	const float yScale = 1.0f / std::tan(fovY * 0.5f);
	const float xScale = yScale / aspectRatio;

	result.m[0][0] = xScale;
	result.m[1][1] = yScale;

	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;

	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
	result.m[3][3] = 0.0f;

	return result;
}

Mat4x4 MAT4x4::OrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
{
	float width = right - left;
	float height = top - bottom;
	float zLength = farClip - nearClip;

	Mat4x4 result{};
	result.m[0][0] = 2 / width;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = 2 / height;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1 / zLength;
	result.m[2][3] = 0.0f;

	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);
	result.m[3][3] = 1;

	return result;
}

Mat4x4 MAT4x4::ViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
	Mat4x4 result{};
	result.m[0][0] = width / 2;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = -height / 2;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = maxDepth - minDepth;
	result.m[2][3] = 0.0f;

	result.m[3][0] = left + (width / 2);
	result.m[3][1] = top + (height / 2);
	result.m[3][2] = minDepth;
	result.m[3][3] = 1.0f;

	return result;
}

Mat4x4 MAT4x4::RotateAxisAngle(const Vec3f& axis, float angle)
{
	float cosTheta = cosf(angle);
	float sinTheta = sinf(angle);

	Mat4x4 result{};
	result = Mat4x4::Identity();

	result.m[0][0] = (axis.x * axis.x) * (1 - cosTheta) + cosTheta;
	result.m[0][1] = (axis.x * axis.y) * (1 - cosTheta) - axis.z * sinTheta;
	result.m[0][2] = (axis.x * axis.z) * (1 - cosTheta) + axis.y * sinTheta;

	result.m[1][0] = (axis.x * axis.y) * (1 - cosTheta) + axis.z * sinTheta;
	result.m[1][1] = (axis.y * axis.y) * (1 - cosTheta) + cosTheta;
	result.m[1][2] = (axis.y * axis.z) * (1 - cosTheta) - axis.x * sinTheta;

	result.m[2][0] = (axis.x * axis.z) * (1 - cosTheta) - axis.y * sinTheta;
	result.m[2][1] = (axis.y * axis.z) * (1 - cosTheta) + axis.x * sinTheta;
	result.m[2][2] = (axis.z * axis.z) * (1 - cosTheta) + cosTheta;

	return result;
}

Mat4x4 MAT4x4::RotateAxisAngle(const Vec3f& axis, float cos, float sin)
{
	Mat4x4 result{};
	result = Mat4x4::Identity();

	result.m[0][0] = axis.x * axis.x * (1 - cos) + cos;
	result.m[0][1] = axis.x * axis.y * (1 - cos) - axis.z * sin;
	result.m[0][2] = axis.x * axis.z * (1 - cos) + axis.y * sin;

	result.m[1][0] = axis.x * axis.y * (1 - cos) + axis.z * sin;
	result.m[1][1] = axis.y * axis.y * (1 - cos) + cos;
	result.m[1][2] = axis.y * axis.z * (1 - cos) - axis.x * sin;

	result.m[2][0] = axis.x * axis.z * (1 - cos) - axis.y * sin;
	result.m[2][1] = axis.y * axis.z * (1 - cos) + axis.x * sin;
	result.m[2][2] = axis.z * axis.z * (1 - cos) + cos;

	return result;
}

Mat4x4 MAT4x4::DirectionToDirection(const Vec3f& from, const Vec3f& to)
{
	Vec3f fromVector = from.Normalized();
	Vec3f toVector = to.Normalized();
	Vec3f n = VEC3::Cross(fromVector, toVector).Normalized();

	float cos = VEC3::Dot(fromVector, toVector);
	float sin = VEC3::Cross(fromVector, toVector).Length();

	if (from.x == -to.x && from.y == -to.y && from.z == -to.z) {

		if (from.x != 0.0f || from.y != 0.0f) {

			n = { from.y, -from.x, 0.0f };
		}
		else if (from.x != 0.0f || from.z != 0.0f) {

			n = { from.z, 0.0f, -from.x };
		}
	}

	Mat4x4 result = RotateAxisAngle(n, sin, cos);

	return result;
}

Mat4x4 MAT4x4::LookAtLH(const Vec3f& eye, const Vec3f& at, const Vec3f& up)
{
	// 前方向（Z+ が前になる）
	Vec3f zAxis = (at - eye).Normalized();

	// 右方向
	Vec3f xAxis = VEC3::Cross(up, zAxis).Normalized();

	// 上方向（直交化）
	Vec3f yAxis = VEC3::Cross(zAxis, xAxis);

	Mat4x4 m{};

	// 行ベクトル前提（row-major）
	m.m[0][0] = xAxis.x;
	m.m[0][1] = yAxis.x;
	m.m[0][2] = zAxis.x;
	m.m[0][3] = 0.0f;

	m.m[1][0] = xAxis.y;
	m.m[1][1] = yAxis.y;
	m.m[1][2] = zAxis.y;
	m.m[1][3] = 0.0f;

	m.m[2][0] = xAxis.z;
	m.m[2][1] = yAxis.z;
	m.m[2][2] = zAxis.z;
	m.m[2][3] = 0.0f;

	// 平行移動（-dot(axis, eye)）
	m.m[3][0] = -VEC3::Dot(xAxis, eye);
	m.m[3][1] = -VEC3::Dot(yAxis, eye);
	m.m[3][2] = -VEC3::Dot(zAxis, eye);
	m.m[3][3] = 1.0f;

	return m;
}

Mat4x4 MAT4x4::OrthoLH(float width, float height, float zn, float zf)
{
	Mat4x4 m{};

	const float invW = 1.0f / width;
	const float invH = 1.0f / height;
	const float invD = 1.0f / (zf - zn);

	// 行ベクトル前提（row-major）
	m.m[0][0] = 2.0f * invW;
	m.m[0][1] = 0.0f;
	m.m[0][2] = 0.0f;
	m.m[0][3] = 0.0f;

	m.m[1][0] = 0.0f;
	m.m[1][1] = 2.0f * invH;
	m.m[1][2] = 0.0f;
	m.m[1][3] = 0.0f;

	m.m[2][0] = 0.0f;
	m.m[2][1] = 0.0f;
	m.m[2][2] = invD;
	m.m[2][3] = 0.0f;

	m.m[3][0] = 0.0f;
	m.m[3][1] = 0.0f;
	m.m[3][2] = -zn * invD;
	m.m[3][3] = 1.0f;

	return m;
}
