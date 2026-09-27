// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1, Code/wwlib/xstraw.cpp and cstraw.cpp.
// Original notices retained in third_party/ea/wwlib.
#include "yrpp/Straws.h"
#include <algorithm>
#include <new>

FileStraw::~FileStraw() {
    if (File && HasOpened) File->Close();
}
int FileStraw::Get(void* output, int length) {
    if (!File || !output || length <= 0) return 0;
    if (!File->HasHandle()) {
        HasOpened = true;
        if (!File->Exists() || !File->Open(FileAccessMode::Read)) return 0;
    }
    return File->ReadBytes(output, length);
}
CacheStraw::CacheStraw(int size) : Buffer(size) {
    if (size <= 0 || !Buffer.Buffer) throw std::bad_alloc();
}
int CacheStraw::Get(void* output, int length) {
    if (!output || length <= 0) return 0;
    int total = 0;
    auto* out = static_cast<char*>(output);
    while (length > 0) {
        if (Length > 0) {
            const int count = std::min(Length, length);
            std::memmove(out, static_cast<char*>(Buffer.Buffer) + Index, size_t(count));
            out += count; Index += count; total += count; Length -= count; length -= count;
        }
        if (!length) break;
        Length = Straw::Get(Buffer.Buffer, Buffer.Size);
        Index = 0;
        if (Length <= 0) break;
    }
    return total;
}
