// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1, Code/wwlib/lzopipe.cpp.
// YR 55C2B0 and 55C720/55C7C0 use BlockSize as the safety margin.
#include "yrpp/Pipes.h"
#include "LZOPipeCodec.hpp"
#include "PipesCompression.hpp"
#include "yrpp/Memory.h"
#include <new>
#include <stdexcept>

LZOPipe::LZOPipe(int control, int blockSize) : Control(control), Counter(0), Buffer(nullptr), Buffer2(nullptr),
    BlockSize(blockSize), SafetyMargin(blockSize),
    BlockHeader_CompCount(-1), BlockHeader_UncompCount(0) {
    if ((control != 0 && control != 1) || blockSize < 1 || blockSize > 64000)
        throw std::invalid_argument("LZOPipe control/block size");
    const int capacity = std::max(blockSize + SafetyMargin, lzo_capacity(blockSize));
    Buffer = YRMemory::Allocate(std::size_t(capacity));
    Buffer2 = YRMemory::Allocate(std::size_t(capacity));
    if (!Buffer || !Buffer2) {
        YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2);
        throw std::bad_alloc();
    }
}
LZOPipe::~LZOPipe() { YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2); }
int LZOPipe::Put(const void* input, int length) { return compressed_put<LZOCodec>(*this, input, length); }
int LZOPipe::Flush() { return compressed_flush<LZOCodec>(*this); }
