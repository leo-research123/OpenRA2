// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1 Code/wwlib/pcx.cpp Read_PCX_File.
// YR 630310 calibration: unsigned header fields, 3-plane RGB->16-bit path,
// RLE marker >= C0, padded scanlines, and original file/Surface lifetimes.
#include "yrpp/Memory.h"
#include "yrpp/FileClass.h"
#include "PCXHelpers.hpp"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <new>

namespace {
std::uint16_t word(const BYTE* bytes) { return std::uint16_t(bytes[0] | (bytes[1] << 8)); }
class PCXInput {
public:
    explicit PCXInput(FileClass* file) : file_(file) { file_->ReadBytes(pool_, 2048); }
    BYTE next() {
        const BYTE value = pool_[position_++];
        if (position_ == 2048) { file_->ReadBytes(pool_, 2048); position_ = 0; }
        return value;
    }
private:
    FileClass* file_;
    BYTE pool_[2048];
    unsigned position_ = 0;
};
}

BSurface* Read_PCX_File(FileClass* file, BytePalette* palette, void* buffer, std::uint32_t size) {
    if (!file->Exists(false)) return nullptr;
    file->Open(FileAccessMode::Read);
    BYTE header[128];
    file->ReadBytes(header, 128);
    // This AND is also in the target. A stricter modern PCX validator would
    // change which original calls succeed. Failed header checks do not Close.
    if (header[0] != 10 && header[1] != 5 && header[3] != 8) return nullptr;
    const std::uint32_t width = std::uint32_t(word(header + 8)) - word(header + 4) + 1;
    int height = int(word(header + 10)) - word(header + 6) + 1;
    const unsigned planes = header[65];
    const unsigned stride = word(header + 66);
    const int bpp = planes == 1 ? 1 : 2;
    if (buffer) height = std::min(height, std::bit_cast<std::int32_t>(size / width - 1));
    // BSurface borrows the caller's pixels, never the address of a temporary
    // MemoryBuffer. Both targets use the same constructor and ownership rules.
    auto* surface = BSurface::Create(std::bit_cast<std::int32_t>(width), height, bpp, buffer);
    if (!surface) return nullptr;
    auto* destination = static_cast<BYTE*>(surface->Lock(0, 0));
    if (destination) {
        PCXInput input(file);
        if (planes == 3) {
            auto* row = static_cast<BYTE*>(YRMemory::Allocate(planes * stride));
            if (!row) {
                surface->Unlock();
                BSurface::Destroy(surface);
                return nullptr; // original leaves the file open on this failure
            }
            for (int y = 0; y < height; ++y) {
                unsigned position = 0;
                do {
                    const BYTE token = input.next();
                    if ((token & 0xc0) == 0xc0) {
                        const BYTE value = input.next();
                        BYTE run = token & 63;
                        do { row[position++] = value; } while (--run);
                    } else row[position++] = token;
                } while (position < planes * stride);
                for (unsigned x = 0; x < width; ++x) {
                    const WORD pixel = Drawing::RGB_To_Int(row[x], row[stride + x], row[2 * stride + x]);
                    std::memcpy(destination, &pixel, 2);
                    destination += 2;
                }
            }
            YRMemory::Deallocate(row);
        } else if (stride == width) {
            const std::uint32_t total = width * std::uint32_t(height);
            std::uint32_t position = 0;
            while (position < total) {
                const BYTE token = input.next();
                if ((token & 0xc0) == 0xc0) {
                    const BYTE value = input.next();
                    const unsigned run = token & 63;
                    std::memset(destination + position, value, run);
                    position += run;
                } else destination[position++] = token;
            }
        } else {
            unsigned position = 0;
            BYTE last = 0;
            for (int y = 0; y < height; ++y) {
                position = 0;
                while (position < stride) {
                    last = input.next();
                    if ((last & 0xc0) == 0xc0) {
                        last &= 63;
                        const BYTE value = input.next();
                        for (unsigned i = 0; i < last; ++i)
                            if (position + i < width) destination[position + i] = value;
                        position += last;
                    } else if (position < width) destination[position++] = last;
                }
                destination += width;
            }
            // Preserve the original final look-ahead and pool refill timing.
            if (position == width) last = input.next();
            if ((last & 0xc0) == 0xc0) input.next();
        }
        surface->Unlock();
    }
    if (palette && planes == 1) {
        file->Seek(-768, FileSeekMode::End);
        file->ReadBytes(palette, 768);
    }
    file->Close();
    return surface;
}
