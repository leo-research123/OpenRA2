// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// WWLib LCW block protocol and lzostraw.cpp pull-state model, adjusted to
// YR 5523E0/552490/5525F0 (no same-class source in the pinned WWLib subset).
#include "yrpp/Straws.h"
#include "LCWPipeCodec.hpp"
#include "PipesCompression.hpp"
#include "yrpp/Memory.h"
#include <new>
#include <stdexcept>

LCWStraw::LCWStraw(int control, int blockSize) : Control(control), Counter(0), Buffer(nullptr), Buffer2(nullptr),
    BlockSize(blockSize), SafetyMargin(blockSize / 128 + 1), BlockHeader_CompCount(0), BlockHeader_UncompCount(0) {
    if ((control != 0 && control != 1) || blockSize < 1 || blockSize > 64000)
        throw std::invalid_argument("LCWStraw control/block size");
    Buffer = YRMemory::Allocate(std::size_t(std::max(blockSize + SafetyMargin, lcw_capacity(blockSize))));
    if (!control) Buffer2 = YRMemory::Allocate(std::size_t(lcw_capacity(blockSize) + 4));
    if (!Buffer || (!control && !Buffer2)) {
        YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2); throw std::bad_alloc();
    }
}
LCWStraw::~LCWStraw() { YRMemory::Deallocate(Buffer); YRMemory::Deallocate(Buffer2); }
int LCWStraw::Get(void* output, int length) { return compressed_get<LCWCodec>(*this, output, length); }
