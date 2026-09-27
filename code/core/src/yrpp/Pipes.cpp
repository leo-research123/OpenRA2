// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from pinned EA WWLib xpipe.cpp.
#include "yrpp/Pipes.h"
FilePipe::~FilePipe() { End(); }
int FilePipe::Put(const void* input, int length) {
    if (!File || !input || length <= 0) return 0;
    if (!File->HasHandle()) {
        HasOpened = true;
        if (!File->Open(FileAccessMode::Write)) return 0;
    }
    return File->WriteBytes(const_cast<void*>(input), length);
}
int FilePipe::End() {
    const int count = Flush();
    if (File && HasOpened) { File->Close(); HasOpened = false; }
    return count;
}
