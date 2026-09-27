// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1, Code/wwlib/lcwpipe.cpp.
// Target 551FF0/552060/5520A0/5522D0: original fields, block state and Flush.
#include "yrpp/Pipes.h"
#include "LCWPipeCodec.hpp"
#include "PipesCompression.hpp"
#include "yrpp/Memory.h"
#include <new>
#include <stdexcept>

LCWPipe::LCWPipe(int control, int blockSize) : Control(control), Counter(0), Buffer(nullptr), Buffer2(nullptr),
    BlockSize(blockSize), SafetyMargin(std::max(blockSize / 128 + 1, 128)),
    BlockHeader_CompCount(-1), BlockHeader_UncompCount(0) {
    if ((control != 0 && control != 1) || blockSize < 1 || blockSize > 64000)
        throw std::invalid_argument("LCWPipe control/block size");
    const int capacity = std::max(blockSize + SafetyMargin, lcw_capacity(blockSize));
    Buffer = YRMemory::Allocate(std::size_t(capacity));
    Buffer2 = YRMemory::Allocate(std::size_t(capacity));
    if (!Buffer || !Buffer2) {
        YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2);
        throw std::bad_alloc();
    }
}
LCWPipe::~LCWPipe() { YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2); }
int LCWPipe::Put(const void* input, int length) { return compressed_put<LCWCodec>(*this, input, length); }
int LCWPipe::Flush() { return compressed_flush<LCWCodec>(*this); }
