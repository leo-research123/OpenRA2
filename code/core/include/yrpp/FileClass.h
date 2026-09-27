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

/* $Header: /CounterStrike/WWFILE.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : WWFILE.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : August 8, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Declarations migrated from YRpp CCFileClass.h; see SOURCE.json and PORTING.md.
#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/Helpers/EnumFlags.h"
#include <cstddef>

enum class FileAccessMode : unsigned int { None = 0, Read = 1, Write = 2, ReadWrite = 3 };
MAKE_ENUM_FLAGS(FileAccessMode);
enum class FileSeekMode : unsigned int { Set = 0, Current = 1, End = 2 };

class FileClass {
public:
    /// VA: unknown.
    FileClass();
    virtual ~FileClass() = default;
    /// VA: implementation-defined (pure virtual).
    virtual const char* GetFileName() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual const char* SetFileName(const char* filename) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual BOOL CreateFile() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual BOOL DeleteFile() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual bool Exists(bool writeShared = false) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual bool HasHandle() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual bool Open(FileAccessMode access = FileAccessMode::Read) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual bool OpenEx(const char* filename, FileAccessMode access = FileAccessMode::Read) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual int ReadBytes(void* buffer, int count) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual int Seek(int offset, FileSeekMode seek = FileSeekMode::Current) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual int GetFileSize() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual int WriteBytes(void* buffer, int count) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void Close() = 0;
    virtual DWORD GetFileTime(); // VA: 0x0065C5F0
    virtual bool SetFileTime(DWORD); // VA: 0x0065C600
    /// VA: implementation-defined (pure virtual).
    virtual void CDCheck(DWORD error, bool retry = false, const char* filename = nullptr) = 0;

    // Migrated in FileClass.cpp; original 0x4A3890, original read-result semantics.
    /// VA: 0x004A3890.
    void* ReadWholeFile();
    template<typename T> bool Read(T& object, int size = sizeof(T)) {
        return ReadBytes(&object, size) == size;
    }
    template<typename T> bool Write(T& object, int size = sizeof(T)) {
        return WriteBytes(&object, size) == size;
    }
protected:
    // Original construction tag: derived original wrappers initialize fields.
    explicit FileClass(noinit_t) {}
public:
    bool SkipCDCheck;
private:
    BYTE padding_5[3];
};
