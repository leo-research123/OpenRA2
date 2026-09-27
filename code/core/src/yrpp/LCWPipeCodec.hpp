// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Internal LCW algorithms shared by the original Pipe/Straw modules.
// Port of pinned WWLib lcw.cpp, calibrated against YR 551C60/551E50.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace {
int lcw_capacity(int block) { return block + (block + 62) / 63 + 5; }
int lcw_word(const unsigned char* p) { return int(p[0]) | (int(p[1]) << 8); }
void lcw_word(unsigned char* p, int n) { p[0] = static_cast<unsigned char>(n); p[1] = static_cast<unsigned char>(n >> 8); }
bool lcw_decode(const void* input, int length, void* output, int expected) {
    const auto* src = static_cast<const unsigned char*>(input);
    auto* dst = static_cast<unsigned char*>(output);
    int in = 0, out = 0;
    while (in < length) {
        const int op = src[in++];
        if (op == 0x80) return out == expected && in == length;
        int count = 0, offset = 0;
        if (!(op & 0x80)) {
            if (in == length) return false;
            count = (op >> 4) + 3;
            offset = out - ((op & 15) * 256 + src[in++]);
        } else if (!(op & 0x40)) {
            count = op & 63;
            if (count > length - in || count > expected - out) return false;
            std::memmove(dst + out, src + in, std::size_t(count));
            in += count; out += count;
            continue;
        } else if (op == 0xfe) {
            if (length - in < 3) return false;
            count = lcw_word(src + in);
            if (count > expected - out) return false;
            std::memset(dst + out, src[in + 2], std::size_t(count));
            in += 3; out += count;
            continue;
        } else {
            if (op == 0xff) {
                if (length - in < 4) return false;
                count = lcw_word(src + in); in += 2;
            } else count = (op & 63) + 3;
            if (length - in < 2) return false;
            offset = lcw_word(src + in); in += 2;
        }
        if (count > expected - out || offset < 0 || offset >= out) return false;
        // Back references may overlap and repeat a short pattern.
        for (int i = 0; i < count; ++i) dst[out++] = dst[offset++];
    }
    return false;
}
int lcw_encode(const void* input, int length, void* output) {
    const auto* src = static_cast<const unsigned char*>(input);
    auto* dst = static_cast<unsigned char*>(output);
    if (!length) { dst[0] = 0x80; return 1; }
    int pos = 1, out = 2, literal = 0;
    bool in_literal = true;
    dst[0] = 0x81; dst[1] = src[0];
    while (pos < length) {
        int search = 0, best = 1, match = 0;
        for (;;) {
            if (length - pos > 64 && src[pos] == src[pos + 64]) {
                int run = 0;
                while (pos + run < length && src[pos + run] == src[pos]) ++run;
                // The target's REP SCASB / DEC EDI leaves the final byte of an
                // end-of-input run for the subsequent match/literal command.
                if (pos + run == length) --run;
                if (run >= 65) {
                    dst[out++] = 0xfe; lcw_word(dst + out, run); out += 2;
                    dst[out++] = src[pos]; pos += run; in_literal = false;
                    continue;
                }
            }
            while (search < pos && src[search] != src[pos]) ++search;
            if (search == pos) break;
            const int candidate = search++;
            if (best > length - pos || src[pos + best - 1] != src[candidate + best - 1]) continue;
            int count = 0;
            while (pos + count < length && src[pos + count] == src[candidate + count]) ++count;
            // Target chooses the latest match when lengths tie.
            if (count >= best) { best = count; match = candidate; }
        }
        if (best <= 2) {
            if (!in_literal || dst[literal] == 0xbf) { literal = out++; dst[literal] = 0x80; }
            ++dst[literal]; dst[out++] = src[pos++]; in_literal = true;
        } else {
            const int distance = pos - match;
            if (best <= 10 && distance <= 0xfff) {
                dst[out++] = static_cast<unsigned char>(((best - 3) << 4) | (distance >> 8));
                dst[out++] = static_cast<unsigned char>(distance);
            } else {
                if (best > 64) { dst[out++] = 0xff; lcw_word(dst + out, best); out += 2; }
                else dst[out++] = static_cast<unsigned char>(0xc0 | (best - 3));
                lcw_word(dst + out, match); out += 2;
            }
            pos += best; in_literal = false;
        }
    }
    dst[out++] = 0x80;
    return out;
}
struct LCWCodec {
    static int Capacity(int block) { return lcw_capacity(block); }
    static int Encode(const void* input, int length, void* output) { return lcw_encode(input, length, output); }
    static bool Decode(const void* input, int length, void* output, int expected) { return lcw_decode(input, length, output, expected); }
};
}
