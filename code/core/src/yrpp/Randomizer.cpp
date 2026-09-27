/*
 * Adapted from EA CnC_Renegade, Code/wwlib/random.cpp and random.h,
 * revision 3e00c3a1b97381bb28be89a35b856375e0629a08.
 * Copyright 2025 Electronic Arts Inc. GPL-3.0-or-later.
 * See third_party/ea/wwlib and third_party/ea/RENEGADE_README.md.
 * Random2/Random3 and Pick_Random_Number retain their original algorithms.
 * YR calibration: 65C6D0/65C780/65C7E0, disable byte and x86 signed shifts.
 */
#include "yrpp/Randomizer.h"
#include <algorithm>
#include <bit>
#include <cstdint>

namespace {
constexpr DWORD mix1[]{0xbaa96887u, 0x1e17d32cu, 0x03bcdc3cu, 0x0f33d1b2u};
constexpr DWORD mix2[]{0x4b0f3b58u, 0xe874f0c3u, 0x6955c5a6u, 0x55a7ca46u};
DWORD signed_high(DWORD value) {
    return static_cast<DWORD>(std::bit_cast<std::int32_t>(value) >> 16);
}
}
Randomizer::Randomizer(DWORD seed) : unknown_00(false), Next1(0), Next2(103) {
    for (DWORD index = 0; index < 250; ++index) {
        DWORD low = seed, high = index;
        for (unsigned round = 0; round < 4; ++round) {
            const DWORD previous = high;
            const DWORD value = high ^ mix1[round];
            const DWORD lo = value & 0xffffu, hi = signed_high(value);
            const DWORD squared = lo * lo + ~(hi * hi);
            // The original uses SAR, not an unsigned rotate.
            const DWORD mixed = signed_high(squared) | (squared << 16);
            high = low ^ ((mixed ^ mix2[round]) + lo * hi);
            low = previous;
        }
        Table[index] = high;
    }
}
int Randomizer::Random() {
    if (unknown_00) return 0;
    Table[Next1] ^= Table[Next2];
    const int value = std::bit_cast<std::int32_t>(Table[Next1]);
    if (++Next1 >= 250) Next1 = 0;
    if (++Next2 >= 250) Next2 = 0;
    return value;
}
int Randomizer::RandomRanged(int minimum, int maximum) {
    if (minimum == maximum) return minimum;
    if (minimum > maximum) std::swap(minimum, maximum);
    const DWORD magnitude = static_cast<DWORD>(maximum) - static_cast<DWORD>(minimum);
    // Original accepted input: the inclusive span fits below INT_MAX.
    // Preserve its signed overflow shortcut for magnitude == INT_MAX.
    const int signed_magnitude = std::bit_cast<std::int32_t>(magnitude);
    unsigned high = 31;
    while (high && !(magnitude & (1u << high))) --high;
    const DWORD mask = ~(0xffffffffu << ((high + 1) & 31));
    int pick = std::bit_cast<std::int32_t>(magnitude + 1u);
    while (pick > signed_magnitude) pick = std::bit_cast<std::int32_t>(static_cast<DWORD>(Random()) & mask);
    return std::bit_cast<std::int32_t>(static_cast<DWORD>(minimum) + static_cast<DWORD>(pick));
}
