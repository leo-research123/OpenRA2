#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace game {
// Westwood MIX's two 40-byte RSA blocks recover a 56-byte Blowfish key.
std::array<uint8_t, 56> mix_key(std::span<const uint8_t, 80> source);
}
