#pragma once
#include <type_traits>

namespace Tsumi::Math {

// =============================
// Vec3
// =============================
template <typename T>
struct Vec2 {

    static_assert(std::is_arithmetic_v<T>, "Vec2<T> requires arithmetic type");

    // メンバ変数
    T x, y;

    // =========================
    // コンストラクタ
    // =========================
    constexpr Vec2() : x(0), y(0) {}
    constexpr Vec2(T _x, T _y) : x(_x), y(_y) {}

    // =========================
    // 比較演算
    // =========================
    constexpr bool operator==(const Vec2& v) const noexcept { return x == v.x && y == v.y; }
    constexpr bool operator!=(const Vec2& v) const noexcept { return !(*this == v); }

    // =========================
    // 単項演算
    // =========================
    constexpr Vec2 operator+() const noexcept { return { +x, +y }; }
    constexpr Vec2 operator-() const noexcept { return { -x, -y }; }

    // =========================
    // ベクトル同士の二項演算
    // =========================
    constexpr Vec2 operator+(const Vec2& v) const noexcept { return { x + v.x, y + v.y }; }
    constexpr Vec2 operator-(const Vec2& v) const noexcept { return { x - v.x, y - v.y }; }
    constexpr Vec2 operator*(const Vec2& v) const noexcept { return { x * v.x, y * v.y }; }
    constexpr Vec2 operator/(const Vec2& v) const noexcept { return { x / v.x, y / v.y }; }
    Vec2& operator+=(const Vec2& v) noexcept { x += v.x; y += v.y; return *this; }
    Vec2& operator-=(const Vec2& v) noexcept { x -= v.x; y -= v.y; return *this; }
    Vec2& operator*=(const Vec2& v) noexcept { x *= v.x; y *= v.y; return *this; }
    Vec2& operator/=(const Vec2& v) noexcept { x /= v.x; y /= v.y; return *this; }

    // =========================
    // スカラー演算
    // =========================
    constexpr Vec2 operator+(T s) const noexcept { return { x + s, y + s }; }
    constexpr Vec2 operator-(T s) const noexcept { return { x - s, y - s }; }
    constexpr Vec2 operator*(T s) const noexcept { return { x * s, y * s }; }
    constexpr Vec2 operator/(T s) const noexcept { return { x / s, y / s }; }
    Vec2& operator+=(T s) noexcept { x += s; y += s; return *this; }
    Vec2& operator-=(T s) noexcept { x -= s; y -= s; return *this; }
    Vec2& operator*=(T s) noexcept { x *= s; y *= s; return *this; }
    Vec2& operator/=(T s) noexcept { x /= s; y /= s; return *this; }

    // =========================
    // 比較 with スカラー
    // =========================
    constexpr bool operator==(T s) const noexcept { return x == s && y == s; }
    constexpr bool operator!=(T s) const noexcept { return !(*this == s); }

    // =========================
    // 外部スカラーとの演算をサポートする非メンバ関数はfriend関数としても可
    // =========================
    friend constexpr Vec2 operator+(T s, const Vec2& v) noexcept { return v + s; }
    friend constexpr Vec2 operator-(T s, const Vec2& v) noexcept { return { s - v.x, s - v.y }; }
    friend constexpr Vec2 operator*(T s, const Vec2& v) noexcept { return v * s; }
    friend constexpr Vec2 operator/(T s, const Vec2& v) noexcept { return { s / v.x, s / v.y }; }

    // =========================
    // 基本関数 
    // =========================
    T Length() const noexcept { return static_cast<T>(std::sqrt(x * x + y * y)); }
    Vec2 Normalize() const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            T len = Length();
            if (len == static_cast<T>(0)) return { 0, 0 };
            return { x / len, y / len };
        }
        else {
            // 整数型では正規化しない
            return *this;
        }
    }
    static T Dot(const Vec2& v1, const Vec2& v2) noexcept {
        return (v1.x * v2.x) + (v1.y * v2.y);
    }
    static T Cross(const Vec2& v1, const Vec2& v2) noexcept {
        // 2Dベクトルの外積はスカラー（z成分相当）を返す
        return (v1.x * v2.y) - (v1.y * v2.x);
    }
    static Vec2 Lerp(const Vec2& a, const Vec2& b, T t) noexcept {
        return a + (b - a) * t;
    }
    T Distance(const Vec2& v) const noexcept {
        return (*this - v).Length();
    }
    static Vec2 Clamp(const Vec2& v, T min, T max) noexcept {
        return { std::clamp(v.x, min, max), std::clamp(v.y, min, max) };
    }
};

// =============================
// Vec3
// =============================
template <typename T>
struct Vec3 {
    static_assert(std::is_arithmetic_v<T>, "Vec3<T> requires arithmetic type");

    T x, y, z;

    // =========================
    // コンストラクタ
    // =========================
    constexpr Vec3() : x(0), y(0), z(0) {}
    constexpr Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

    // --- 比較演算 ---

    // =========================
    // 比較演算
    // =========================
    constexpr bool operator==(const Vec3& v) const noexcept { return x == v.x && y == v.y && z == v.z; }
    constexpr bool operator!=(const Vec3& v) const noexcept { return !(*this == v); }
    constexpr bool operator==(T s) const noexcept { return x == s && y == s && z == s; }
    constexpr bool operator!=(T s) const noexcept { return !(*this == s); }

    // =========================
    // 単項演算
    // =========================
    constexpr Vec3 operator+() const noexcept { return { +x, +y, +z }; }
    constexpr Vec3 operator-() const noexcept { return { -x, -y, -z }; }

    // =========================
    // ベクトル同士の二項演算
    // =========================
    constexpr Vec3 operator+(const Vec3& v) const noexcept { return { x + v.x, y + v.y, z + v.z }; }
    constexpr Vec3 operator-(const Vec3& v) const noexcept { return { x - v.x, y - v.y, z - v.z }; }
    constexpr Vec3 operator*(const Vec3& v) const noexcept { return { x * v.x, y * v.y, z * v.z }; }
    constexpr Vec3 operator/(const Vec3& v) const noexcept { return { x / v.x, y / v.y, z / v.z }; }
    Vec3& operator+=(const Vec3& v) noexcept { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3& v) noexcept { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3& operator*=(const Vec3& v) noexcept { x *= v.x; y *= v.y; z *= v.z; return *this; }
    Vec3& operator/=(const Vec3& v) noexcept { x /= v.x; y /= v.y; z /= v.z; return *this; }

    // =========================
    // スカラー演算
    // =========================
    constexpr Vec3 operator+(T s) const noexcept { return { x + s, y + s, z + s }; }
    constexpr Vec3 operator-(T s) const noexcept { return { x - s, y - s, z - s }; }
    constexpr Vec3 operator*(T s) const noexcept { return { x * s, y * s, z * s }; }
    constexpr Vec3 operator/(T s) const noexcept { return { x / s, y / s, z / s }; }
    Vec3& operator+=(T s) noexcept { x += s; y += s; z += s; return *this; }
    Vec3& operator-=(T s) noexcept { x -= s; y -= s; z -= s; return *this; }
    Vec3& operator*=(T s) noexcept { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(T s) noexcept { x /= s; y /= s; z /= s; return *this; }

    // =========================
    // 外部スカラーとの演算をサポートするfriend関数
    // =========================
    friend constexpr Vec3 operator+(T s, const Vec3& v) noexcept { return v + s; }
    friend constexpr Vec3 operator-(T s, const Vec3& v) noexcept { return { s - v.x, s - v.y, s - v.z }; }
    friend constexpr Vec3 operator*(T s, const Vec3& v) noexcept { return v * s; }
    friend constexpr Vec3 operator/(T s, const Vec3& v) noexcept { return { s / v.x, s / v.y, s / v.z }; }

    // =========================
    // 基本関数
    // =========================
    T Length() const noexcept { return static_cast<T>(std::sqrt(x * x + y * y + z * z)); }
    Vec3 Normalize() const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            T len = Length();
            if (len == static_cast<T>(0)) return { 0, 0, 0 };
            return { x / len, y / len, z / len };
        }
        else {
            return *this; // int型ではそのまま
        }
    }
    static Vec3 Clamp(T min, T max) noexcept {
        std::clamp(x, min, max);
        std::clamp(y, min, max);
        std::clamp(z, min, max);
    }
};


// =============================
// Vec4
// =============================
template <typename T>
struct Vec4 {
    static_assert(std::is_arithmetic_v<T>, "Vec4<T> requires arithmetic type");

    T x, y, z, w;

    // =========================
    // コンストラクタ
    // =========================
    constexpr Vec4() : x(0), y(0), z(0), w(0) {}
    constexpr Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

    // =========================
    // 比較演算
    // =========================
    constexpr bool operator==(const Vec4& v) const noexcept { return x == v.x && y == v.y && z == v.z && w == v.w; }
    constexpr bool operator!=(const Vec4& v) const noexcept { return !(*this == v); }
    constexpr bool operator==(T s) const noexcept { return x == s && y == s && z == s && w == s; }
    constexpr bool operator!=(T s) const noexcept { return !(*this == s); }

    // =========================
    // 単項演算
    // =========================
    constexpr Vec4 operator+() const noexcept { return { +x, +y, +z, +w }; }
    constexpr Vec4 operator-() const noexcept { return { -x, -y, -z, -w }; }

    // =========================
    // ベクトル同士の二項演算
    // =========================
    constexpr Vec4 operator+(const Vec4& v) const noexcept { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
    constexpr Vec4 operator-(const Vec4& v) const noexcept { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
    constexpr Vec4 operator*(const Vec4& v) const noexcept { return { x * v.x, y * v.y, z * v.z, w * v.w }; }
    constexpr Vec4 operator/(const Vec4& v) const noexcept { return { x / v.x, y / v.y, z / v.z, w / v.w }; }
    Vec4& operator+=(const Vec4& v) noexcept { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vec4& operator-=(const Vec4& v) noexcept { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    Vec4& operator*=(const Vec4& v) noexcept { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    Vec4& operator/=(const Vec4& v) noexcept { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

    // =========================
    // スカラー演算
    // =========================
    constexpr Vec4 operator+(T s) const noexcept { return { x + s, y + s, z + s, w + s }; }
    constexpr Vec4 operator-(T s) const noexcept { return { x - s, y - s, z - s, w - s }; }
    constexpr Vec4 operator*(T s) const noexcept { return { x * s, y * s, z * s, w * s }; }
    constexpr Vec4 operator/(T s) const noexcept { return { x / s, y / s, z / s, w / s }; }
    Vec4& operator+=(T s) noexcept { x += s; y += s; z += s; w += s; return *this; }
    Vec4& operator-=(T s) noexcept { x -= s; y -= s; z -= s; w -= s; return *this; }
    Vec4& operator*=(T s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
    Vec4& operator/=(T s) noexcept { x /= s; y /= s; z /= s; w /= s; return *this; }

    // =========================
    // 外部スカラーとの演算（左右両対応）
    // =========================
    friend constexpr Vec4 operator+(T s, const Vec4& v) noexcept { return v + s; }
    friend constexpr Vec4 operator-(T s, const Vec4& v) noexcept { return { s - v.x, s - v.y, s - v.z, s - v.w }; }
    friend constexpr Vec4 operator*(T s, const Vec4& v) noexcept { return v * s; }
    friend constexpr Vec4 operator/(T s, const Vec4& v) noexcept { return { s / v.x, s / v.y, s / v.z, s / v.w }; }

    // =========================
    // 基本関数
    // =========================
    T Length() const noexcept { return static_cast<T>(std::sqrt(x * x + y * y + z * z + w * w)); }
    Vec4 Normalize() const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            T len = Length();
            if (len == static_cast<T>(0)) return { 0, 0, 0, 0 };
            return { x / len, y / len, z / len, w / len };
        }
        else {
            return *this;
        }
    }
    static T Dot(const Vec4& a, const Vec4& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
    static T Distance(const Vec4& a, const Vec4& b) noexcept {
        return (a - b).Length();
    }
    static Vec4 Clamp(const Vec4& v, T min, T max) noexcept {
        return {
            std::clamp(v.x, min, max),
            std::clamp(v.y, min, max),
            std::clamp(v.z, min, max),
            std::clamp(v.w, min, max)
        };
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