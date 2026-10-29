#include "mix_crypto.hpp"
#include "yrpp/MixFileClass.h"
#include <algorithm>
namespace game {
std::array<uint8_t, 56> mix_key(std::span<const uint8_t, 80> source) {
    std::array<uint8_t, 78> decoded{};
    MixFileClass::DefaultKey().Decrypt(source.data(), int(source.size()), decoded.data());
    std::array<uint8_t, 56> result{};
    std::copy_n(decoded.begin(), result.size(), result.begin());
    return result;
}
}
