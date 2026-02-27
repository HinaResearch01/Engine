#pragma once

#include <cmath>
#include <cstring> 
#include <algorithm>
#include <cassert>
#include "../Vector/Vector.h"

namespace tme::math {

// =============================
// 3x3 Matrix
// =============================
struct Mat3x3 {
    float m[3][3];

	// =========================
	// コンストラクタ
	// =========================
	Mat3x3(float a11, float a12, float a13,
			  float a21, float a22, float a23,
			  float a31, float a32, float a33) {
		m[0][0] = a11; m[0][1] = a12; m[0][2] = a13;
		m[1][0] = a21; m[1][1] = a22; m[1][2] = a23;
		m[2][0] = a31; m[2][1] = a32; m[2][2] = a33;
	}
	Mat3x3() : m{
		{(0.0f, 0.0f, 0.0f)},
		{(0.0f, 0.0f, 0.0f)},
		{(0.0f, 0.0f, 0.0f)},
	} {
	};

	// =========================
	// 行列操作
	// =========================
	// ===== Factory =====
	[[nodiscard]] static Mat3x3 Identity() {
		Mat3x3 mat{};
		mat.m[0][0] = mat.m[1][1] = mat.m[2][2] = 1.0f;
		return mat;
	}
	[[nodiscard]] static Mat3x3 Scale(const Vec2f& s) {
		Mat3x3 mat = Identity();
		mat.m[0][0] = s.x;
		mat.m[1][1] = s.y;
		return mat;
	}
	[[nodiscard]] static Mat3x3 RotationX(float angle) {
		Mat3x3 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[1][1] = c;
		mat.m[1][2] = s;
		mat.m[2][1] = -s;
		mat.m[2][2] = c;
		return mat;
	}
	[[nodiscard]] static Mat3x3 RotationY(float angle) {
		Mat3x3 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[0][0] = c;
		mat.m[0][2] = -s;
		mat.m[2][0] = s;
		mat.m[2][2] = c;
		return mat;
	}
	[[nodiscard]] static Mat3x3 RotationZ(float angle) {
		Mat3x3 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[0][0] = c;
		mat.m[0][1] = s;
		mat.m[1][0] = -s;
		mat.m[1][1] = c;
		return mat;
	}
	[[nodiscard]] static Mat3x3 Rotation(float rad)
	{
		float c = std::cos(rad);
		float s = std::sin(rad);

		// 行ベクトル用（右手系）
		return {
			 c,  s, 0,
			-s,  c, 0,
			 0,  0, 1
		};
	}
	[[nodiscard]] static Mat3x3 Translation(const Vec2f& t)
	{
		return {
			1, 0, 0,
			0, 1, 0,
			t.x, t.y, 1
		};
	}

	// ===== Mutating =====


	// ===== Math =====
	// 行列変換
	[[nodiscard]] Mat3x3 Transpose() const {
		Mat3x3 result{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				result.m[i][j] = m[j][i];
		return result;
	}
	[[nodiscard]] Mat3x3 Inverse() const {
		Mat3x3 inv{};
		float det =
			m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
			m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
			m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

		if (det == 0.0f) return Identity(); // 特異行列は単位行列返す

		float invDet = 1.0f / det;

		inv.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
		inv.m[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * invDet;
		inv.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;

		inv.m[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * invDet;
		inv.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
		inv.m[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * invDet;

		inv.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
		inv.m[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * invDet;
		inv.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;

		return inv;
	}
	// ベクトル変換
	[[nodiscard]] Vec3f TransformVector(const Vec3f& v) const {
		return {
			v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
			v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
			v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
		};
	}

	// =========================
	// 単項演算
	// =========================
	Mat3x3 operator+() const {
		return {
			+m[0][0], +m[0][1], +m[0][2],
			+m[1][0], +m[1][1], +m[1][2],
			+m[2][0], +m[2][1], +m[2][2]
		};
	}
	Mat3x3 operator-() const {
		return {
			-m[0][0], -m[0][1], -m[0][2],
			-m[1][0], -m[1][1], -m[1][2],
			-m[2][0], -m[2][1], -m[2][2]
		};
	}
	// 行列同士の演算
	[[nodiscard]] Mat3x3 operator+(const Mat3x3& rhs) const {
		Mat3x3 result{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				result.m[i][j] = m[i][j] + rhs.m[i][j];
		return result;
	}
	Mat3x3& operator+=(const Mat3x3& rhs) {
		*this = *this + rhs;
		return *this;
	}
	[[nodiscard]] Mat3x3 operator-(const Mat3x3& rhs) const {
		Mat3x3 result{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				result.m[i][j] = m[i][j] - rhs.m[i][j];
		return result;
	}
	Mat3x3& operator-=(const Mat3x3& rhs) {
		*this = *this - rhs;
		return *this;
	}
	[[nodiscard]] Mat3x3 operator*(const Mat3x3& rhs) const {
		Mat3x3 result{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				result.m[i][j] =
				m[i][0] * rhs.m[0][j] +
				m[i][1] * rhs.m[1][j] +
				m[i][2] * rhs.m[2][j];
		return result;
	}
	Mat3x3& operator*=(const Mat3x3& rhs) {
		*this = *this * rhs;
		return *this;
	}
	// スカラー演算
	Mat3x3 operator*(float s) const {
		Mat3x3 result{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				result.m[i][j] = m[i][j] * s;
		return result;
	}
	Mat3x3& operator*=(float s) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				m[i][j] *= s;
		return *this;
	}
	Mat3x3 operator/(float s) const {
		Mat3x3 result{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				result.m[i][j] = m[i][j] / s;
		return result;
	}
	Mat3x3& operator/=(float s) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				m[i][j] /= s;
		return *this;
	}
	// スカラー左側演算
	friend Mat3x3 operator*(float s, const Mat3x3& mat) {
		return mat * s;
	}
};


// =============================
// 4x4 Matrix
// =============================
struct Mat4x4 {
	float m[4][4];

	// =========================
	// コンストラクタ
	// =========================
	Mat4x4(
		float a11, float a12, float a13, float a14,
		float a21, float a22, float a23, float a24,
		float a31, float a32, float a33, float a34,
		float a41, float a42, float a43, float a44) {
		m[0][0] = a11; m[0][1] = a12; m[0][2] = a13; m[0][3] = a14;
		m[1][0] = a21; m[1][1] = a22; m[1][2] = a23; m[1][3] = a24;
		m[2][0] = a31; m[2][1] = a32; m[2][2] = a33; m[2][3] = a34;
		m[3][0] = a41; m[3][1] = a42; m[3][2] = a43; m[3][3] = a44;
	}
	Mat4x4() : m{
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
	} {}

	// =========================
	// Raw Access
	// =========================
	[[nodiscard]] const float* GetRaw() const
	{
		return &m[0][0];
	}
	[[nodiscard]] float* GetRaw()
	{
		return &m[0][0];
	}
	[[nodiscard]] Vec4f GetRow(int i) const
	{
		return { m[i][0], m[i][1], m[i][2], m[i][3] };
	}
	[[nodiscard]] Vec4f GetColumn(int j) const
	{
		return { m[0][j], m[1][j], m[2][j], m[3][j] };
	}
	Vec3f GetRight() const { return { m[0][0], m[0][1], m[0][2] }; }
	Vec3f GetUp() const { return { m[1][0], m[1][1], m[1][2] }; }
	Vec3f GetForward() const { return { m[2][0], m[2][1], m[2][2] }; }
	Vec3f GetPosition() const { return { m[3][0], m[3][1], m[3][2] }; }

	// =========================
	// 行列操作
	// =========================
	// ===== Factory =====
	[[nodiscard]] static Mat4x4 Identity()
	{
		return Mat4x4(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);
	}
	[[nodiscard]] static Mat4x4 Scale(const Vec3f& s)
	{
		Mat4x4 mat = Identity();
		mat.m[0][0] = s.x;
		mat.m[1][1] = s.y;
		mat.m[2][2] = s.z;
		return mat;
	}
	[[nodiscard]] static Mat4x4 RotationX(float angle)
	{
		Mat4x4 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[1][1] = c;
		mat.m[1][2] = -s;
		mat.m[2][1] = s;
		mat.m[2][2] = c;
		return mat;
	}
	[[nodiscard]] static Mat4x4 RotationY(float angle)
	{
		Mat4x4 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[0][0] = c;
		mat.m[0][2] = s;
		mat.m[2][0] = -s;
		mat.m[2][2] = c;
		return mat;
	}
	[[nodiscard]] static Mat4x4 RotationZ(float angle)
	{
		Mat4x4 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[0][0] = c;
		mat.m[0][1] = -s;
		mat.m[1][0] = s;
		mat.m[1][1] = c;
		return mat;
	}
	[[nodiscard]] static Mat4x4 Rotation(const Vec3f& t)
	{
		Mat4x4 x = RotationX(t.x);
		Mat4x4 y = RotationY(t.y);
		Mat4x4 z = RotationZ(t.z);
		return x * (y * z);
	}
	[[nodiscard]] static Mat4x4 Translation(const Vec3f& t)
	{
		Mat4x4 mat = Identity();
		mat.m[3][0] = t.x;
		mat.m[3][1] = t.y;
		mat.m[3][2] = t.z;
		return mat;
	}

	// ===== Mutating =====
	void SetIdentity() {}
	void ApplyScale(const Vec3f&) {}
	void ApplyRotation(const Vec3f&) {}
	void ApplyTranslation(const Vec3f&) {}

	// ===== Math =====
	// 行列変換
	[[nodiscard]] Mat4x4 Transpose() const
	{
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = m[j][i];
		return result;
	}
	[[nodiscard]] Mat4x4 Inverse() const
	{
		float as =
			m[0][0] * m[1][1] * m[2][2] * m[3][3] +
			m[0][0] * m[1][2] * m[2][3] * m[3][1] +
			m[0][0] * m[1][3] * m[2][1] * m[3][2] -

			m[0][0] * m[1][3] * m[2][2] * m[3][1] -
			m[0][0] * m[1][2] * m[2][1] * m[3][3] -
			m[0][0] * m[1][1] * m[2][3] * m[3][2] -

			m[0][1] * m[1][0] * m[2][2] * m[3][3] -
			m[0][2] * m[1][0] * m[2][3] * m[3][1] -
			m[0][3] * m[1][0] * m[2][1] * m[3][2] +

			m[0][3] * m[1][0] * m[2][2] * m[3][1] +
			m[0][2] * m[1][0] * m[2][1] * m[3][3] +
			m[0][1] * m[1][0] * m[2][3] * m[3][2] +

			m[0][1] * m[1][2] * m[2][0] * m[3][3] +
			m[0][2] * m[1][3] * m[2][0] * m[3][1] +
			m[0][3] * m[1][1] * m[2][0] * m[3][2] -

			m[0][3] * m[1][2] * m[2][0] * m[3][1] -
			m[0][2] * m[1][1] * m[2][0] * m[3][3] -
			m[0][1] * m[1][3] * m[2][0] * m[3][2] -

			m[0][1] * m[1][2] * m[2][3] * m[3][0] -
			m[0][2] * m[1][3] * m[2][1] * m[3][0] -
			m[0][3] * m[1][1] * m[2][2] * m[3][0] +

			m[0][3] * m[1][2] * m[2][1] * m[3][0] +
			m[0][2] * m[1][1] * m[2][3] * m[3][0] +
			m[0][1] * m[1][3] * m[2][2] * m[3][0];

		assert(as != 0.0f);
		float determinantRecp = 1.0f / as;

		Mat4x4 result{};

		// 一行目
		result.m[0][0] = (m[1][1] * m[2][2] * m[3][3] + m[1][2] * m[2][3] * m[3][1] +
						  m[1][3] * m[2][1] * m[3][2] - m[1][3] * m[2][2] * m[3][1] -
						  m[1][2] * m[2][1] * m[3][3] - m[1][1] * m[2][3] * m[3][2]) *
			determinantRecp;

		result.m[0][1] = (-m[0][1] * m[2][2] * m[3][3] - m[0][2] * m[2][3] * m[3][1] -
						  m[0][3] * m[2][1] * m[3][2] + m[0][3] * m[2][2] * m[3][1] +
						  m[0][2] * m[2][1] * m[3][3] + m[0][1] * m[2][3] * m[3][2]) *
			determinantRecp;

		result.m[0][2] = (
			m[0][1] * m[1][2] * m[3][3] + m[0][2] * m[1][3] * m[3][1] +
			m[0][3] * m[1][1] * m[3][2] - m[0][3] * m[1][2] * m[3][1] -
			m[0][2] * m[1][1] * m[3][3] - m[0][1] * m[1][3] * m[3][2]) *
			determinantRecp;

		result.m[0][3] = (-m[0][1] * m[1][2] * m[2][3] - m[0][2] * m[1][3] * m[2][1] -
						  m[0][3] * m[1][1] * m[2][2] + m[0][3] * m[1][2] * m[2][1] +
						  m[0][2] * m[1][1] * m[2][3] + m[0][1] * m[1][3] * m[2][2]) *
			determinantRecp;


		// 二行目
		result.m[1][0] = (-m[1][0] * m[2][2] * m[3][3] - m[1][2] * m[2][3] * m[3][0] -
						  m[1][3] * m[2][0] * m[3][2] + m[1][3] * m[2][2] * m[3][0] +
						  m[1][2] * m[2][0] * m[3][3] + m[1][0] * m[2][3] * m[3][2]) *
			determinantRecp;

		result.m[1][1] = (
			m[0][0] * m[2][2] * m[3][3] + m[0][2] * m[2][3] * m[3][0] +
			m[0][3] * m[2][0] * m[3][2] - m[0][3] * m[2][2] * m[3][0] -
			m[0][2] * m[2][0] * m[3][3] - m[0][0] * m[2][3] * m[3][2]) *
			determinantRecp;

		result.m[1][2] = (-m[0][0] * m[1][2] * m[3][3] - m[0][2] * m[1][3] * m[3][0] -
						  m[0][3] * m[1][0] * m[3][2] + m[0][3] * m[1][2] * m[3][0] +
						  m[0][2] * m[1][0] * m[3][3] + m[0][0] * m[1][3] * m[3][2]) *
			determinantRecp;

		result.m[1][3] = (
			m[0][0] * m[1][2] * m[2][3] + m[0][2] * m[1][3] * m[2][0] +
			m[0][3] * m[1][0] * m[2][2] - m[0][3] * m[1][2] * m[2][0] -
			m[0][2] * m[1][0] * m[2][3] - m[0][0] * m[1][3] * m[2][2]) *
			determinantRecp;


		// 三行目
		result.m[2][0] = (
			m[1][0] * m[2][1] * m[3][3] + m[1][1] * m[2][3] * m[3][0] +
			m[1][3] * m[2][0] * m[3][1] - m[1][3] * m[2][1] * m[3][0] -
			m[1][1] * m[2][0] * m[3][3] - m[1][0] * m[2][3] * m[3][1]) *
			determinantRecp;

		result.m[2][1] = (-m[0][0] * m[2][1] * m[3][3] - m[0][1] * m[2][3] * m[3][0] -
						  m[0][3] * m[2][0] * m[3][1] + m[0][3] * m[2][1] * m[3][0] +
						  m[0][1] * m[2][0] * m[3][3] + m[0][0] * m[2][3] * m[3][1]) *
			determinantRecp;

		result.m[2][2] = (
			m[0][0] * m[1][1] * m[3][3] + m[0][1] * m[1][3] * m[3][0] +
			m[0][3] * m[1][0] * m[3][1] - m[0][3] * m[1][1] * m[3][0] -
			m[0][1] * m[1][0] * m[3][3] - m[0][0] * m[1][3] * m[3][1]) *
			determinantRecp;

		result.m[2][3] = (-m[0][0] * m[1][1] * m[2][3] - m[0][1] * m[1][3] * m[2][0] -
						  m[0][3] * m[1][0] * m[2][1] + m[0][3] * m[1][1] * m[2][0] +
						  m[0][1] * m[1][0] * m[2][3] + m[0][0] * m[1][3] * m[2][1]) *
			determinantRecp;


		// 四行目
		result.m[3][0] = (-m[1][0] * m[2][1] * m[3][2] - m[1][1] * m[2][2] * m[3][0] -
						  m[1][2] * m[2][0] * m[3][1] + m[1][2] * m[2][1] * m[3][0] +
						  m[1][1] * m[2][0] * m[3][2] + m[1][0] * m[2][2] * m[3][1]) *
			determinantRecp;

		result.m[3][1] = (

			m[0][0] * m[2][1] * m[3][2] + m[0][1] * m[2][2] * m[3][0] +
			m[0][2] * m[2][0] * m[3][1] - m[0][2] * m[2][1] * m[3][0] -
			m[0][1] * m[2][0] * m[3][2] - m[0][0] * m[2][2] * m[3][1]) *
			determinantRecp;

		result.m[3][2] = (-m[0][0] * m[1][1] * m[3][2] - m[0][1] * m[1][2] * m[3][0] -
						  m[0][2] * m[1][0] * m[3][1] + m[0][2] * m[1][1] * m[3][0] +
						  m[0][1] * m[1][0] * m[3][2] + m[0][0] * m[1][2] * m[3][1]) *
			determinantRecp;

		result.m[3][3] = (

			m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] +
			m[0][2] * m[1][0] * m[2][1] - m[0][2] * m[1][1] * m[2][0] -
			m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1]) *
			determinantRecp;

		return result;
	}
	// ベクトル変換
	[[nodiscard]] Vec3f TransformVector(const Vec3f& v) const
	{
		return {
			v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
			v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
			v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
		};
	}
	[[nodiscard]] Vec3f TransformPoint(const Vec3f& v) const
	{
		float x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0];
		float y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1];
		float z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2];
		return { x, y, z };
	}

	// =========================
	// 単項演算
	// =========================
	Mat4x4 operator+() const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = +m[i][j];
		return result;
	}
	Mat4x4 operator-() const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = -m[i][j];
		return result;
	}
	// 行列同士の演算
	[[nodiscard]] Mat4x4 operator+(const Mat4x4& rhs) const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = m[i][j] + rhs.m[i][j];
		return result;
	}
	Mat4x4& operator+=(const Mat4x4& rhs) {
		*this = *this + rhs;
		return *this;
	}
	[[nodiscard]] Mat4x4 operator-(const Mat4x4& rhs) const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = m[i][j] - rhs.m[i][j];
		return result;
	}
	Mat4x4& operator-=(const Mat4x4& rhs) {
		*this = *this - rhs;
		return *this;
	}
	[[nodiscard]] Mat4x4 operator*(const Mat4x4& rhs) const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] =
					m[i][0] * rhs.m[0][j] +
					m[i][1] * rhs.m[1][j] +
					m[i][2] * rhs.m[2][j] +
					m[i][3] * rhs.m[3][j];
			}
		}
		return result;
	}
	Mat4x4& operator*=(const Mat4x4& rhs) {
		*this = *this * rhs;
		return *this;
	}
	// スカラー演算
	Mat4x4 operator*(float s) const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = m[i][j] * s;
		return result;
	}
	Mat4x4& operator*=(float s) {
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				m[i][j] *= s;
		return *this;
	}
	Mat4x4 operator/(float s) const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = m[i][j] / s;
		return result;
	}
	Mat4x4& operator/=(float s) {
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				m[i][j] /= s;
		return *this;
	}
	// スカラー左側演算
	friend Mat4x4 operator*(float s, const Mat4x4& mat) {
		return mat * s;
	}
	// 
	friend Vec4f operator*(const Mat4x4& m, const Vec4f& v)
	{
		return {
			m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w,
			m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w,
			m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w,
			m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w,
		};
	}

	friend Vec4f operator*(const Vec4f& v, const Mat4x4& m)
	{
		return {
			v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0],
			v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1],
			v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2],
			v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3],
		};
	}
};

}