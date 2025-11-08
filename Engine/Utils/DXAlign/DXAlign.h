#pragma once

#include <cstddef>
#include <stdint.h>

namespace Tsumi::Utils {

/// <summary>
/// 任意のアラインメントに切り上げ（例: AlignUp(300, 256) → 512）
/// </summary>
constexpr size_t AlignUp(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/// <summary>
/// DirectX12 の CBV 用 256バイトアライン関数
/// </summary>
constexpr size_t AlignCB(size_t size)
{
    constexpr size_t CB_ALIGNMENT = 256;
    return AlignUp(size, CB_ALIGNMENT);
}

}