// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Shared original WWLib LCW/LZO block-state protocol (lcwpipe/lzopipe/lzostraw).
// The original classes retain their own layouts and virtual slots.
#pragma once
#include "yrpp/Pipes.h"
#include "yrpp/Straws.h"
#include "yrpp/Memory.h"
#include <algorithm>
#include <memory>
namespace {
int block_word(const unsigned char* p) { return int(p[0]) | (int(p[1]) << 8); }
void block_word(unsigned char* p, int n) { p[0] = static_cast<unsigned char>(n); p[1] = static_cast<unsigned char>(n >> 8); }
template<class Codec, class T> int compressed_put(T& self, const void* input, int length) {
    if (self.Counter < 0) return -1;
    if (!input || length < 1) return self.Pipe::Put(input, length);
    auto* buffer = static_cast<unsigned char*>(self.Buffer);
    const auto* source = static_cast<const unsigned char*>(input);
    int total = 0;
    auto emit = [&](const void* bytes, int count) {
        const int size = Codec::Encode(bytes, count, self.Buffer2);
        if (size < 0) { self.Counter = -1; return false; }
        self.BlockHeader_CompCount = static_cast<short>(size); self.BlockHeader_UncompCount = static_cast<short>(count);
        unsigned char header[4]; block_word(header, size); block_word(header + 2, count);
        total += self.Pipe::Put(header, 4); total += self.Pipe::Put(self.Buffer2, size);
        return true;
    };
    if (self.Control == 1) {
        while (length > 0) {
            if (self.BlockHeader_CompCount == -1) {
                const int count = std::min(length, 4 - self.Counter);
                std::memmove(buffer + self.Counter, source, std::size_t(count));
                self.Counter += count; source += count; length -= count;
                if (self.Counter != 4) break;
                self.BlockHeader_CompCount = static_cast<short>(block_word(buffer));
                self.BlockHeader_UncompCount = static_cast<short>(block_word(buffer + 2));
                self.Counter = 0;
                const int compressed = static_cast<unsigned short>(self.BlockHeader_CompCount);
                const int plain = static_cast<unsigned short>(self.BlockHeader_UncompCount);
                if (compressed <= 0 || compressed == 0xffff || compressed > Codec::Capacity(self.BlockSize) || plain > self.BlockSize) {
                    self.Counter = -1; return -1;
                }
            }
            if (!length) break;
            const int compressed = static_cast<unsigned short>(self.BlockHeader_CompCount);
            const int count = std::min(length, compressed - self.Counter);
            std::memmove(buffer + self.Counter, source, std::size_t(count));
            self.Counter += count; source += count; length -= count;
            if (self.Counter == compressed) {
                const int plain = static_cast<unsigned short>(self.BlockHeader_UncompCount);
                if (!Codec::Decode(self.Buffer, compressed, self.Buffer2, plain)) { self.Counter = -1; return -1; }
                total += self.Pipe::Put(self.Buffer2, plain); self.Counter = 0; self.BlockHeader_CompCount = -1;
            }
        }
    } else {
        if (self.Counter > 0) {
            const int count = std::min(length, self.BlockSize - self.Counter);
            std::memmove(buffer + self.Counter, source, std::size_t(count));
            self.Counter += count; source += count; length -= count;
            if (self.Counter == self.BlockSize) { if (!emit(self.Buffer, self.BlockSize)) return -1; self.Counter = 0; }
        }
        while (length >= self.BlockSize) { if (!emit(source, self.BlockSize)) return -1; source += self.BlockSize; length -= self.BlockSize; }
        if (length > 0) { std::memmove(self.Buffer, source, std::size_t(length)); self.Counter = length; }
    }
    return total;
}
template<class Codec, class T> int compressed_flush(T& self) {
    if (self.Counter < 0) return -1;
    int total = 0;
    if (self.Counter > 0) {
        unsigned char header[4];
        if (self.Control == 1) {
            if (self.BlockHeader_CompCount != -1) {
                block_word(header, static_cast<unsigned short>(self.BlockHeader_CompCount));
                block_word(header + 2, static_cast<unsigned short>(self.BlockHeader_UncompCount));
                total += self.Pipe::Put(header, 4);
            }
            // Original partial-stream Flush deliberately passes bytes through.
            total += self.Pipe::Put(self.Buffer, self.Counter);
            self.BlockHeader_CompCount = -1;
        } else {
            const int size = Codec::Encode(self.Buffer, self.Counter, self.Buffer2);
            if (size < 0) { self.Counter = -1; return -1; }
            self.BlockHeader_CompCount = static_cast<short>(size); self.BlockHeader_UncompCount = static_cast<short>(self.Counter);
            block_word(header, size); block_word(header + 2, self.Counter);
            total += self.Pipe::Put(header, 4); total += self.Pipe::Put(self.Buffer2, size);
        }
        self.Counter = 0;
    }
    return total + self.Pipe::Flush();
}
template<class Codec, class T> int compressed_get(T& self, void* output, int length) {
    if (self.Counter < 0) return -1;
    if (!output || length < 1) return 0;
    auto* out = static_cast<unsigned char*>(output);
    int total = 0;
    while (length > 0) {
        if (self.Counter > 0) {
            const int count = std::min(length, self.Counter);
            const auto* bytes = self.Control == 1 ? static_cast<unsigned char*>(self.Buffer) +
                static_cast<unsigned short>(self.BlockHeader_UncompCount) - self.Counter :
                static_cast<unsigned char*>(self.Buffer2) + static_cast<unsigned short>(self.BlockHeader_CompCount) + 4 - self.Counter;
            std::memmove(out, bytes, std::size_t(count));
            self.Counter -= count; out += count; length -= count; total += count;
        }
        if (!length) break;
        if (self.Control == 1) {
            unsigned char header[4];
            const int got = self.Straw::Get(header, 4);
            if (got == 0) break;
            if (got != 4) { self.Counter = -1; return -1; }
            const int compressed = block_word(header), plain = block_word(header + 2);
            self.BlockHeader_CompCount = static_cast<short>(compressed); self.BlockHeader_UncompCount = static_cast<short>(plain);
            if (!compressed || compressed > Codec::Capacity(self.BlockSize) || plain > self.BlockSize) { self.Counter = -1; return -1; }
            // Keep compressed input separate from output so bounded decoding also
            // accepts literal-heavy LCW blocks without in-place overlap hazards.
            std::unique_ptr<unsigned char, decltype(&YRMemory::Deallocate)> encoded(
                static_cast<unsigned char*>(YRMemory::Allocate(std::size_t(compressed))), YRMemory::Deallocate);
            if (!encoded) { self.Counter = -1; return -1; }
            const int count = self.Straw::Get(encoded.get(), compressed);
            const bool ok = count == compressed && Codec::Decode(encoded.get(), compressed, self.Buffer, plain);
            if (!ok) { self.Counter = -1; return -1; }
            self.Counter = plain;
        } else {
            const int count = self.Straw::Get(self.Buffer, self.BlockSize);
            if (count == 0) break;
            if (count < 0 || count > self.BlockSize) { self.Counter = -1; return -1; }
            auto* bytes = static_cast<unsigned char*>(self.Buffer2);
            const int size = Codec::Encode(self.Buffer, count, bytes + 4);
            if (size < 0) { self.Counter = -1; return -1; }
            self.BlockHeader_CompCount = static_cast<short>(size); self.BlockHeader_UncompCount = static_cast<short>(count);
            block_word(bytes, size); block_word(bytes + 2, count); self.Counter = size + 4;
        }
    }
    return total;
}
}
