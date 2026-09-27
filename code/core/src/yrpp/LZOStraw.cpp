// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1, Code/wwlib/lzostraw.cpp; YR 55C720/55C7C0.
#include "yrpp/Straws.h"
#include "LZOPipeCodec.hpp"
#include "PipesCompression.hpp"
#include "yrpp/Memory.h"
#include <new>
#include <stdexcept>

LZOStraw::LZOStraw(int control, int blockSize) : Control(control), Counter(0), Buffer(nullptr), Buffer2(nullptr),
    BlockSize(blockSize), SafetyMargin(blockSize), BlockHeader_CompCount(0), BlockHeader_UncompCount(0) {
    if ((control != 0 && control != 1) || blockSize < 1 || blockSize > 64000)
        throw std::invalid_argument("LZOStraw control/block size");
    Buffer = YRMemory::Allocate(std::size_t(std::max(blockSize + SafetyMargin, lzo_capacity(blockSize))));
    if (!control) Buffer2 = YRMemory::Allocate(std::size_t(lzo_capacity(blockSize) + 4));
    if (!Buffer || (!control && !Buffer2)) {
        YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2); throw std::bad_alloc();
    }
}
LZOStraw::~LZOStraw() { YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2); }
int LZOStraw::Get(void* output, int length) { return compressed_get<LZOCodec>(*this, output, length); }
