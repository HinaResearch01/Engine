#pragma once

#include <string_view>
#include <cstdint>

namespace tme::util::Hash {

using Hash64 = uint64_t;

constexpr Hash64 FNV_OFFSET = 1469598103934665603ull;
constexpr Hash64 FNV_PRIME = 1099511628211ull;

inline Hash64 FNV1a64(std::string_view s)
{
	Hash64 h = FNV_OFFSET;
	for (char c : s) {
		h ^= static_cast<uint8_t>(c);
		h *= FNV_PRIME;
	}
	return h;
}

}