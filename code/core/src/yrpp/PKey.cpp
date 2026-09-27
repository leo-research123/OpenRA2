// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from the pinned WWLib pk.cpp and mpmath.cpp (third_party/ea/wwlib).
// Original block/DER contracts and square-and-multiply exponentiation are kept.
// Portable 32-bit limb arithmetic replaces WWLib's unsigned-long/asm backend;
// scratch state is local. No key generation or random-stream dependency is used.
#include "yrpp/MixFileClass.h"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstring>

namespace {
using Number = std::array<uint32_t, 64>;
int bits(const uint32_t* n) noexcept {
    for (int i = 63; i >= 0; --i)
        if (n[i]) return i * 32 + 32 - std::countl_zero(n[i]);
    return 0;
}
int compare(const uint32_t* a, const uint32_t* b, int count = 64) noexcept {
    for (int i = count - 1; i >= 0; --i)
        if (a[i] != b[i]) return a[i] > b[i] ? 1 : -1;
    return 0;
}
void subtract(Number& a, const uint32_t* b, int count) noexcept {
    uint64_t borrow = 0;
    for (int i = 0; i < count; ++i) {
        const uint64_t rhs = uint64_t(b[i]) + borrow;
        borrow = uint64_t(a[i]) < rhs;
        a[i] = uint32_t(uint64_t(a[i]) - rhs);
    }
}
Number add_mod(const Number& a, const Number& b, const uint32_t* modulus, int count) noexcept {
    Number result{};
    uint64_t carry = 0;
    for (int i = 0; i < count; ++i) {
        const uint64_t sum = uint64_t(a[i]) + b[i] + carry;
        result[i] = uint32_t(sum);
        carry = sum >> 32;
    }
    // Inputs are reduced; one subtraction suffices, including a carry limb.
    if (carry || compare(result.data(), modulus, count) >= 0) subtract(result, modulus, count);
    return result;
}
Number multiply_mod(Number a, const Number& b, const uint32_t* modulus, int count) noexcept {
    Number result{};
    const int length = bits(b.data());
    for (int i = 0; i < length; ++i) {
        if ((b[i / 32] >> (i % 32)) & 1) result = add_mod(result, a, modulus, count);
        a = add_mod(a, a, modulus, count);
    }
    return result;
}
Number exponent_mod(const Number& base, const PKey& key) noexcept {
    Number result{};
    result[0] = 1;
    const int exponent_bits = bits(key.Exponent);
    // XMP_Exponent_Mod initializes the result to 1 before these error exits.
    // PKey copies that result even when Int's separate Error state is nonzero.
    if (!exponent_bits || !bits(key.Modulus) || compare(base.data(), key.Modulus) >= 0
        || compare(key.Exponent, key.Modulus) >= 0) return result;
    const int count = (bits(key.Modulus) + 31) / 32;
    result = base;
    for (int i = exponent_bits - 2; i >= 0; --i) {
        result = multiply_mod(result, result, key.Modulus, count);
        if ((key.Exponent[i / 32] >> (i % 32)) & 1)
            result = multiply_mod(result, base, key.Modulus, count);
    }
    return result;
}
void decode(uint32_t* output, const void* der) noexcept {
    if (!der) return;
    const auto* input = static_cast<const uint8_t*>(der);
    if (*input++ != 2) return;
    unsigned length = *input++;
    if (length & 0x80) {
        const unsigned count = length & 0x7f;
        if (count > 2) return;
        length = *input++;
        if (count > 1) length = (length << 8) | *input++;
    }
    if (!length || length > 256) return;
    // DER is signed, big endian; Int<64> sign-extends to all 256 bytes.
    std::fill_n(output, 64, (input[0] & 0x80) ? UINT32_MAX : 0);
    for (unsigned i = 0; i < length; ++i) {
        const auto shift = (i % 4) * 8;
        output[i / 4] = (output[i / 4] & ~(uint32_t(0xff) << shift))
            | uint32_t(input[length - 1 - i]) << shift;
    }
}
int transform(const PKey& key, const void* source, int length, void* destination, bool encrypt) noexcept {
    if (!source || !destination || key.BitPrecision < 9 || key.BitPrecision > 2047) return 0;
    const int plain = key.Plain_Block_Size(), crypt = key.Crypt_Block_Size();
    const int input_size = encrypt ? plain : crypt, output_size = encrypt ? crypt : plain;
    if (length < input_size || int64_t(length / input_size) * output_size > INT_MAX) return 0;
    const auto* input = static_cast<const uint8_t*>(source);
    auto* output = static_cast<uint8_t*>(destination);
    int total = 0;
    for (; length >= input_size; length -= input_size) {
        Number base{};
        for (int i = 0; i < input_size; ++i) base[i / 4] |= uint32_t(input[i]) << (8 * (i % 4));
        const auto decoded = exponent_mod(base, key);
        for (int i = 0; i < output_size; ++i) output[i] = uint8_t(decoded[i / 4] >> (8 * (i % 4)));
        input += input_size;
        output += output_size;
        total += output_size;
    }
    return total;
}
}
PKey::PKey(const void* exponent, const void* modulus) noexcept {
    Decode_Exponent(exponent);
    Decode_Modulus(modulus);
}
void PKey::Decode_Modulus(const void* der) noexcept { decode(Modulus, der); BitPrecision = bits(Modulus) - 1; }
void PKey::Decode_Exponent(const void* der) noexcept { decode(Exponent, der); }
int PKey::Block_Count(int length) const noexcept {
    return BitPrecision >= 9 && BitPrecision <= 2047 && length > 0
        ? (length - 1) / Plain_Block_Size() + 1 : 0;
}
int PKey::Encrypt(const void* source, int length, void* destination) const noexcept {
    return transform(*this, source, length, destination, true);
}
int PKey::Decrypt(const void* source, int length, void* destination) const noexcept {
    return transform(*this, source, length, destination, false);
}
const PKey& MixFileClass::DefaultKey() noexcept {
    // The DER public key embedded in the fixed game and pinned XCC mix_decode.
    static constexpr uint8_t exponent[]{2, 3, 1, 0, 1};
    static constexpr uint8_t modulus[]{2, 40,
        0x51,0xbc,0xda,0x08,0x6d,0x39,0xfc,0xe4,0x56,0x51,0x60,0xd6,0x51,0x71,0x3f,0xa2,
        0xe8,0xaa,0x54,0xfa,0x66,0x82,0xb0,0x4a,0xab,0xdd,0x0e,0x6a,0xf8,0xb0,0xc1,0xe6,
        0xd1,0xfb,0x4f,0x3d,0xaa,0x43,0x7f,0x15};
    static const PKey key(exponent, modulus);
    return key;
}
