// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2000 Olaf van der Spek <olafvdspek@gmail.com>
// TS/RA2 decode3 traversal adapted from Olaf van der Spek's
// XCC in EA CNC_TS_and_RA2_Mission_Editor 6abf0f557469baea73079c6bf6550709e2e3584e:
// 3rdParty/xcc/misc/shp_decode.cpp::decode3.
// Original source and notices: native/third_party/ea/xcc.
// Adaptation: explicit failure, cropped indexed output; no
// editor canvas expansion and no change to SHPStruct::GetPixels semantics.
#include "api/images.hpp"
#include <algorithm>
#include <cstddef>

namespace game {
namespace {
uint16_t u16(const uint8_t* data) {
    return uint16_t(data[0]) | (uint16_t(data[1]) << 8);
}
bool fail(std::string& error, const char* message) { error = message; return false; }
}

bool decode_shp_pixels(const uint8_t* payload, int width, int height,
    bool compressed, std::vector<uint8_t>& indices, std::string& error) {
    if (width < 0 || height < 0 ||
        (height && size_t(width) > (16u * 1024u * 1024u) / size_t(height)))
        return fail(error, "Invalid SHP frame dimensions");
    if (!width || !height) { indices.clear(); error.clear(); return true; }
    if (!payload) return fail(error, "Missing SHP frame pixels");
    std::vector<uint8_t> decoded(size_t(width) * size_t(height), 0);
    if (!compressed) {
        std::copy_n(payload, decoded.size(), decoded.data());
    } else {
        const uint8_t* source = payload;
        for (int y = 0; y < height; ++y) {
            const size_t length = u16(source);
            if (length < 2) return fail(error, "Invalid SHP RLE row length");
            size_t remaining = length - 2;
            source += 2;
            size_t x = 0;
            while (remaining) {
                const uint8_t value = *source++;
                --remaining;
                if (value) {
                    if (x >= size_t(width)) return fail(error, "SHP RLE row exceeds its width");
                    decoded[size_t(y) * size_t(width) + x++] = value;
                } else {
                    if (!remaining) return fail(error, "Truncated SHP transparent run");
                    // Original files (including GI) overshoot the right edge in
                    // their final transparent run. XCC decode3 clips that run.
                    x += std::min(size_t(*source++), size_t(width) - x);
                    --remaining;
                }
            }
            if (x != size_t(width)) return fail(error, "Incomplete SHP RLE row");
        }
    }
    indices = std::move(decoded); error.clear();
    return true;
}
}
