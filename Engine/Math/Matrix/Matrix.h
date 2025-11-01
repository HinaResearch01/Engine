#pragma once
#include <cmath>
#include <cstring> 
#include <algorithm>
#include "../Vecor/Vector.h"
#include <cassert>

namespace Tsumi::Math {

// =============================
// 3x3 Matrix
// =============================
struct Mat3x3 {
    float m[3][3];

	// =========================
	// コンストラクタ
	// =========================
    Mat3x3() {
        Identity();
    }
	Mat3x3(float a11, float a12, float a13,
		float a21, float a22, float a23,
		float a31, float a32, float a33) {
		m[0][0] = a11; m[0][1] = a12; m[0][2] = a13;
		m[1][0] = a21; m[1][1] = a22; m[1][2] = a23;
		m[2][0] = a31; m[2][1] = a32; m[2][2] = a33;
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
	
	// =========================
	// 行列同士の演算
	// =========================
	Mat3x3 operator+(const Mat3x3& rhs) const {
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
	Mat3x3 operator-(const Mat3x3& rhs) const {
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
	Mat3x3 operator*(const Mat3x3& rhs) const {
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
	
	// =========================
	// スカラー演算
	// =========================
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
	
	// =========================
	// スカラー左側演算
	// =========================
	friend Mat3x3 operator*(float s, const Mat3x3& mat) {
		return mat * s;
	}


	// =========================
	// 行列操作
	// =========================
	static Mat3x3 Identity() {
		Mat3x3 mat{};
		mat.m[0][0] = mat.m[1][1] = mat.m[2][2] = 1.0f;
		return mat;
	}
    Mat3x3 Transpose() const {
        Mat3x3 result{};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                result.m[i][j] = m[j][i];
        return result;
    }
    Mat3x3 Inverse() const {
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
    static Mat3x3 Scale(const Vec3f& s) {
        Mat3x3 mat = Identity();
        mat.m[0][0] = s.x;
        mat.m[1][1] = s.y;
        mat.m[2][2] = s.z;
        return mat;
    }
    static Mat3x3 RotationX(float angle) {
        Mat3x3 mat = Identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[1][1] = c;
        mat.m[1][2] = s;
        mat.m[2][1] = -s;
        mat.m[2][2] = c;
        return mat;
    }
    static Mat3x3 RotationY(float angle) {
        Mat3x3 mat = Identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c;
        mat.m[0][2] = -s;
        mat.m[2][0] = s;
        mat.m[2][2] = c;
        return mat;
    }
    static Mat3x3 RotationZ(float angle) {
        Mat3x3 mat = Identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c;
        mat.m[0][1] = s;
        mat.m[1][0] = -s;
        mat.m[1][1] = c;
        return mat;
    }
    Vec3f TransformVector(const Vec3f& v) const {
        return {
            v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
            v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
            v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
        };
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
	Mat4x4() {
		Identity();
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
	// =========================
	// 行列同士の演算
	// =========================
	Mat4x4 operator+(const Mat4x4& rhs) const {
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
	Mat4x4 operator-(const Mat4x4& rhs) const {
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
	Mat4x4 operator*(const Mat4x4& rhs) const {
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
	// =========================
	// スカラー演算
	// =========================
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

	// =========================
	// スカラー左側演算
	// =========================
	friend Mat4x4 operator*(float s, const Mat4x4& mat) {
		return mat * s;
	}

	// =========================
	// 行列操作
	// =========================
	// 単位行列
	static Mat4x4 Identity() {
		Mat4x4 mat{};
		mat.m[0][0] = mat.m[1][1] = mat.m[2][2] = mat.m[3][3] = 1.0f;
		return mat;
	}
	// ベクトル変換（方向ベクトル）
	Vec3f TransformVector(const Vec3f& v) const {
		return {
			v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
			v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
			v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
		};
	}
	// ベクトル変換（位置ベクトル）
	Vec3f TransformPoint(const Vec3f& v) const {
		float x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0];
		float y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1];
		float z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2];
		return { x, y, z };
	}
	// 転置
	Mat4x4 Transpose() const {
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = m[j][i];
		return result;
	}
	// 逆行列（4x4用）
    Mat4x4 Inverse() const {
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

		Mat4x4 result {};

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
	static Mat4x4 Scale(const Vec3f& s) {
		Mat4x4 mat = Identity();
		mat.m[0][0] = s.x;
		mat.m[1][1] = s.y;
		mat.m[2][2] = s.z;
		return mat;
	}
	static Mat4x4 RotationX(float angle) {
		Mat4x4 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[1][1] = c;
		mat.m[1][2] = s;
		mat.m[2][1] = -s;
		mat.m[2][2] = c;
		return mat;
	}
	static Mat4x4 RotationY(float angle) {
		Mat4x4 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[0][0] = c;
		mat.m[0][2] = -s;
		mat.m[2][0] = s;
		mat.m[2][2] = c;
		return mat;
	}
	static Mat4x4 RotationZ(float angle) {
		Mat4x4 mat = Identity();
		float c = std::cos(angle);
		float s = std::sin(angle);
		mat.m[0][0] = c;
		mat.m[0][1] = s;
		mat.m[1][0] = -s;
		mat.m[1][1] = c;
		return mat;
	}
	static Mat4x4 Rotation(const Vec3f& t) {
		Mat4x4 x = RotationX(t.x);
		Mat4x4 y = RotationY(t.y);
		Mat4x4 z = RotationZ(t.z);
		return x * ( y * z );
	}
	static Mat4x4 Translation(const Vec3f& t) {
		Mat4x4 mat = Identity();
		mat.m[3][0] = t.x;
		mat.m[3][1] = t.y;
		mat.m[3][2] = t.z;
		return mat;
	}
};

}