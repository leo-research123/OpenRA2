// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// SHA block expansion/rounds/finalization adapted from pinned CnC_Renegade
// WWLib sha.cpp/sha.h and shapipe.cpp. Kept local to CCINI digest serialization.
// Target 476D80 caches SHA-1 then runs CRCEngine over the 20 digest bytes.
#include "yrpp/CCINIClass.h"
#include "yrpp/CRC.h"
#include "yrpp/Pipes.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>

namespace {
class DigestPipe final : public Pipe {
    std::array<std::uint32_t, 5> state{0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u, 0xc3d2e1f0u};
    std::array<byte, 64> partial{};
    std::uint32_t length = 0;
    size_t pending = 0;

    void ProcessBlock(const byte* source) {
        std::uint32_t words[80];
        for (unsigned i = 0; i < 16; ++i)
            words[i] = (std::uint32_t(source[4*i]) << 24) | (std::uint32_t(source[4*i+1]) << 16)
                | (std::uint32_t(source[4*i+2]) << 8) | source[4*i+3];
        for (unsigned i = 16; i < 80; ++i)
            words[i] = std::rotl(words[i-3] ^ words[i-8] ^ words[i-14] ^ words[i-16], 1);
        auto a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
        for (unsigned i = 0; i < 80; ++i) {
            const std::uint32_t function = i < 20 ? d ^ (b & (c ^ d))
                : i < 40 ? b ^ c ^ d : i < 60 ? (b & c) | (d & (b | c)) : b ^ c ^ d;
            const std::uint32_t constant = i < 20 ? 0x5a827999u : i < 40 ? 0x6ed9eba1u
                : i < 60 ? 0x8f1bbcdcu : 0xca62c1d6u;
            const auto next = std::rotl(a, 5) + function + e + words[i] + constant;
            e = d; d = c; c = std::rotl(b, 30); b = a; a = next;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
    }
public:
    int Put(const void* source, int count) override {
        if (!source || count <= 0) return 0;
        const auto* bytes = static_cast<const byte*>(source);
        auto remaining = size_t(count);
        length += std::uint32_t(count);
        while (remaining) {
            const size_t take = std::min(remaining, partial.size() - pending);
            std::memcpy(partial.data() + pending, bytes, take);
            pending += take; bytes += take; remaining -= take;
            if (pending == partial.size()) { ProcessBlock(partial.data()); pending = 0; }
        }
        return count;
    }
    void Result(byte* digest) {
        partial[pending++] = 0x80;
        if (pending > 56) {
            std::fill(partial.begin() + pending, partial.end(), 0);
            ProcessBlock(partial.data());
            pending = 0;
        }
        std::fill(partial.begin() + pending, partial.end(), 0);
        // Match the original 32-bit WWLib length word (upper length word is zero).
        const std::uint32_t bits = length * 8u;
        for (unsigned i = 0; i < 4; ++i) partial[60+i] = byte(bits >> (24-8*i));
        ProcessBlock(partial.data());
        for (unsigned i = 0; i < 20; ++i) digest[i] = byte(state[i/4] >> (24-8*(i%4)));
    }
};
}

void CCINIClass::CalculateDigest() {
    DigestPipe output;
    WritePipe(output);
    output.Result(Digest);
    Digested = true;
}
DWORD CCINIClass::GetCRC() {
    if (!Digested) CalculateDigest();
    CRCEngine crc;
    return static_cast<DWORD>(crc(Digest, sizeof(Digest)));
}
