#pragma once

#include <type_traits>
#include <cmath>
#include <algorithm>

namespace tme::math {

// =============================
// Vec3
// =============================
template <typename T>
struct Vec2 {
	static_assert(std::is_arithmetic_v<T>, "Vec2<T> requires arithmetic type");

	T x, y;

	// =========================
	// コンストラクタ
	// =========================
	constexpr Vec2() : x(0), y(0) {}
	constexpr Vec2(T _x, T _y) : x(_x), y(_y) {}

	// =========================
	// 比較演算
	// =========================
	[[nodiscard]] constexpr bool operator==(const Vec2& v) const noexcept { return x == v.x && y == v.y; }
	[[nodiscard]] constexpr bool operator!=(const Vec2& v) const noexcept { return !(*this == v); }
	[[nodiscard]] constexpr bool operator==(T s) const noexcept { return x == s && y == s; }
	[[nodiscard]] constexpr bool operator!=(T s) const noexcept { return !(*this == s); }

	// =========================
	// 単項演算
	// =========================
	[[nodiscard]] constexpr Vec2 operator+() const noexcept { return { +x, +y }; }
	[[nodiscard]] constexpr Vec2 operator-() const noexcept { return { -x, -y }; }

	// =========================
	// ベクトル同士の二項演算
	// =========================
	[[nodiscard]] constexpr Vec2 operator+(const Vec2& v) const noexcept { return { x + v.x, y + v.y }; }
	[[nodiscard]] constexpr Vec2 operator-(const Vec2& v) const noexcept { return { x - v.x, y - v.y }; }
	[[nodiscard]] constexpr Vec2 operator*(const Vec2& v) const noexcept { return { x * v.x, y * v.y }; }
	[[nodiscard]] constexpr Vec2 operator/(const Vec2& v) const noexcept { return { x / v.x, y / v.y }; }

	Vec2& operator+=(const Vec2& v) noexcept { x += v.x; y += v.y; return *this; }
	Vec2& operator-=(const Vec2& v) noexcept { x -= v.x; y -= v.y; return *this; }
	Vec2& operator*=(const Vec2& v) noexcept { x *= v.x; y *= v.y; return *this; }
	Vec2& operator/=(const Vec2& v) noexcept { x /= v.x; y /= v.y; return *this; }

	// =========================
	// スカラー演算
	// =========================
	[[nodiscard]] constexpr Vec2 operator+(T s) const noexcept { return { x + s, y + s }; }
	[[nodiscard]] constexpr Vec2 operator-(T s) const noexcept { return { x - s, y - s }; }
	[[nodiscard]] constexpr Vec2 operator*(T s) const noexcept { return { x * s, y * s }; }
	[[nodiscard]] constexpr Vec2 operator/(T s) const noexcept { return { x / s, y / s }; }

	Vec2& operator+=(T s) noexcept { x += s; y += s; return *this; }
	Vec2& operator-=(T s) noexcept { x -= s; y -= s; return *this; }
	Vec2& operator*=(T s) noexcept { x *= s; y *= s; return *this; }
	Vec2& operator/=(T s) noexcept { x /= s; y /= s; return *this; }

	friend [[nodiscard]] constexpr Vec2 operator+(T s, const Vec2& v) noexcept { return v + s; }
	friend [[nodiscard]] constexpr Vec2 operator-(T s, const Vec2& v) noexcept { return { s - v.x, s - v.y }; }
	friend [[nodiscard]] constexpr Vec2 operator*(T s, const Vec2& v) noexcept { return v * s; }
	friend [[nodiscard]] constexpr Vec2 operator/(T s, const Vec2& v) noexcept { return { s / v.x, s / v.y }; }

	// =========================
	// 基本関数
	// =========================
	[[nodiscard]] T Length() const noexcept {
		return static_cast<T>(std::sqrt(x * x + y * y));
	}

	[[nodiscard]] Vec2 Normalize() const noexcept {
		if constexpr (std::is_floating_point_v<T>) {
			T len = Length();
			if (len == static_cast<T>(0)) return { 0, 0 };
			return { x / len, y / len };
		}
		else {
			return *this;
		}
	}

	void Clamp(T min, T max) noexcept {
		x = std::clamp(x, min, max);
		y = std::clamp(y, min, max);
	}
};

// =============================
// Vec3
// =============================
template <typename T>
struct Vec3 {
	static_assert(std::is_arithmetic_v<T>, "Vec3<T> requires arithmetic type");

	T x, y, z;

	constexpr Vec3() : x(0), y(0), z(0) {}
	constexpr Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

	[[nodiscard]] constexpr bool operator==(const Vec3& v) const noexcept { return x == v.x && y == v.y && z == v.z; }
	[[nodiscard]] constexpr bool operator!=(const Vec3& v) const noexcept { return !(*this == v); }
	[[nodiscard]] constexpr bool operator==(T s) const noexcept { return x == s && y == s && z == s; }
	[[nodiscard]] constexpr bool operator!=(T s) const noexcept { return !(*this == s); }

	[[nodiscard]] constexpr Vec3 operator+() const noexcept { return { +x, +y, +z }; }
	[[nodiscard]] constexpr Vec3 operator-() const noexcept { return { -x, -y, -z }; }

	[[nodiscard]] constexpr Vec3 operator+(const Vec3& v) const noexcept { return { x + v.x, y + v.y, z + v.z }; }
	[[nodiscard]] constexpr Vec3 operator-(const Vec3& v) const noexcept { return { x - v.x, y - v.y, z - v.z }; }
	[[nodiscard]] constexpr Vec3 operator*(const Vec3& v) const noexcept { return { x * v.x, y * v.y, z * v.z }; }
	[[nodiscard]] constexpr Vec3 operator/(const Vec3& v) const noexcept { return { x / v.x, y / v.y, z / v.z }; }

	Vec3& operator+=(const Vec3& v) noexcept { x += v.x; y += v.y; z += v.z; return *this; }
	Vec3& operator-=(const Vec3& v) noexcept { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Vec3& operator*=(const Vec3& v) noexcept { x *= v.x; y *= v.y; z *= v.z; return *this; }
	Vec3& operator/=(const Vec3& v) noexcept { x /= v.x; y /= v.y; z /= v.z; return *this; }

	[[nodiscard]] constexpr Vec3 operator+(T s) const noexcept { return { x + s, y + s, z + s }; }
	[[nodiscard]] constexpr Vec3 operator-(T s) const noexcept { return { x - s, y - s, z - s }; }
	[[nodiscard]] constexpr Vec3 operator*(T s) const noexcept { return { x * s, y * s, z * s }; }
	[[nodiscard]] constexpr Vec3 operator/(T s) const noexcept { return { x / s, y / s, z / s }; }

	Vec3& operator+=(T s) noexcept { x += s; y += s; z += s; return *this; }
	Vec3& operator-=(T s) noexcept { x -= s; y -= s; z -= s; return *this; }
	Vec3& operator*=(T s) noexcept { x *= s; y *= s; z *= s; return *this; }
	Vec3& operator/=(T s) noexcept { x /= s; y /= s; z /= s; return *this; }

	friend [[nodiscard]] constexpr Vec3 operator+(T s, const Vec3& v) noexcept { return v + s; }
	friend [[nodiscard]] constexpr Vec3 operator-(T s, const Vec3& v) noexcept { return { s - v.x, s - v.y, s - v.z }; }
	friend [[nodiscard]] constexpr Vec3 operator*(T s, const Vec3& v) noexcept { return v * s; }
	friend [[nodiscard]] constexpr Vec3 operator/(T s, const Vec3& v) noexcept { return { s / v.x, s / v.y, s / v.z }; }

	[[nodiscard]] T Length() const noexcept {
		return static_cast<T>(std::sqrt(x * x + y * y + z * z));
	}

	[[nodiscard]] Vec3 Normalized() const noexcept {
		if constexpr (std::is_floating_point_v<T>) {
			T len = Length();
			if (len == static_cast<T>(0)) return { 0, 0, 0 };
			return { x / len, y / len, z / len };
		}
		else {
			return *this;
		}
	}

	void Clamp(T min, T max) noexcept {
		x = std::clamp(x, min, max);
		y = std::clamp(y, min, max);
		z = std::clamp(z, min, max);
	}
};


// =============================
// Vec4
// =============================
template <typename T>
struct Vec4 {
	static_assert(std::is_arithmetic_v<T>, "Vec4<T> requires arithmetic type");

	T x, y, z, w;

	constexpr Vec4() : x(0), y(0), z(0), w(0) {}
	constexpr Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

	[[nodiscard]] constexpr bool operator==(const Vec4& v) const noexcept { return x == v.x && y == v.y && z == v.z && w == v.w; }
	[[nodiscard]] constexpr bool operator!=(const Vec4& v) const noexcept { return !(*this == v); }

	[[nodiscard]] constexpr Vec4 operator+() const noexcept { return { +x, +y, +z, +w }; }
	[[nodiscard]] constexpr Vec4 operator-() const noexcept { return { -x, -y, -z, -w }; }

	[[nodiscard]] constexpr Vec4 operator+(const Vec4& v) const noexcept { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
	[[nodiscard]] constexpr Vec4 operator-(const Vec4& v) const noexcept { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
	[[nodiscard]] constexpr Vec4 operator*(const Vec4& v) const noexcept { return { x * v.x, y * v.y, z * v.z, w * v.w }; }
	[[nodiscard]] constexpr Vec4 operator/(const Vec4& v) const noexcept { return { x / v.x, y / v.y, z / v.z, w / v.w }; }

	Vec4& operator+=(const Vec4& v) noexcept { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	Vec4& operator-=(const Vec4& v) noexcept { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	Vec4& operator*=(const Vec4& v) noexcept { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	Vec4& operator/=(const Vec4& v) noexcept { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

	[[nodiscard]] T Length() const noexcept {
		return static_cast<T>(std::sqrt(x * x + y * y + z * z + w * w));
	}

	[[nodiscard]] Vec4 Normalize() const noexcept {
		if constexpr (std::is_floating_point_v<T>) {
			T len = Length();
			if (len == static_cast<T>(0)) return { 0, 0, 0, 0 };
			return { x / len, y / len, z / len, w / len };
		}
		else {
			return *this;
		}
	}

	void Clamp(T min, T max) noexcept {
		x = std::clamp(x, min, max);
		y = std::clamp(y, min, max);
		z = std::clamp(z, min, max);
		w = std::clamp(w, min, max);
	}
};


// =============================
// 型エイリアス
// =============================
using Vec2f = Vec2<float>;
using Vec3f = Vec3<float>;
using Vec4f = Vec4<float>;

using Vec2d = Vec2<double>;
using Vec3d = Vec3<double>;
using Vec4d = Vec4<double>;

using Vec2i = Vec2<int>;
using Vec3i = Vec3<int>;
using Vec4i = Vec4<int>;

using Vec2u = Vec2<unsigned int>;
using Vec3u = Vec3<unsigned int>;
using Vec4u = Vec4<unsigned int>;

}