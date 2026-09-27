// Adapted from EA REDALERT at f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// See third_party/ea/README.md for the necessary host changes and original license.
//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

// YRpp CCFileClass.h declarations with the existing EA-derived implementation.
#pragma once

#include "yrpp/FileClass.h"
#include <cstdlib>
#include <climits>
#ifdef _WIN32
using RawFileHandle = HANDLE;
inline const RawFileHandle RAW_NULL_HANDLE = INVALID_HANDLE_VALUE;
#else
using RawFileHandle = int;
inline constexpr RawFileHandle RAW_NULL_HANDLE = -1;
#endif

class RawFileClass : public FileClass {
public:
    /// VA: 0x0065CA80.
    RawFileClass(const char* filename);
    /// VA: unknown.
    RawFileClass(); // host convenience; the game path uses the filename constructor
    RawFileClass(const RawFileClass&) = delete;
    RawFileClass& operator=(const RawFileClass&) = delete;
    /// VA: 0x0065CA00.
    ~RawFileClass() override;
    const char* GetFileName() const override; // VA: 0x00401940
    const char* SetFileName(const char* filename) override; // VA: 0x0065CAC0
    BOOL CreateFile() override; // VA: 0x0065D150
    BOOL DeleteFile() override; // VA: 0x0065D190
    bool Exists(bool writeShared = false) override; // VA: 0x0065CBF0
    bool HasHandle() override; // VA: 0x0065D420
    bool Open(FileAccessMode access = FileAccessMode::Read) override; // VA: 0x0065CB50
    bool OpenEx(const char* filename, FileAccessMode access = FileAccessMode::Read) override; // VA: 0x0065CB30
    int ReadBytes(void* buffer, int count) override; // VA: 0x0065CCE0
    int Seek(int offset, FileSeekMode seek = FileSeekMode::Current) override; // VA: 0x0065CF00
    int GetFileSize() override; // VA: 0x0065D0D0
    int WriteBytes(void* buffer, int count) override; // VA: 0x0065CDD0
    void Close() override; // VA: 0x0065CCA0
    DWORD GetFileTime() override; // VA: 0x0065D1F0
    bool SetFileTime(DWORD time) override; // VA: 0x0065D240
    void CDCheck(DWORD error, bool retry = false, const char* filename = nullptr) override; // VA: 0x0065CA70
    /// VA: 0x0065D2B0.
    void Bias(int offset = 0, int length = -1);

    FileAccessMode FileAccess;
    int FilePointer; // original RawFile bias start, not the current cursor
    int FileSize;    // original RawFile bias length, -1 means unbounded
    RawFileHandle Handle;
    const char* FileName;
    short unknown_short_1C; // Original +1C word; preserve YRpp name and type.
    short unknown_short_1E; // Original +1E word; both initialized to zero at 65CA80.
    bool FileNameAllocated;
private:
    BYTE padding_21[3];
protected:
    explicit RawFileClass(noinit_t);
    /// VA: unknown.
    int RawSeek(int offset, FileSeekMode seek = FileSeekMode::Current);
};
