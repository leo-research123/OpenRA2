#include "filesystem/file_system.hpp"
#include "filesystem/file_names.hpp"
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

#include "yrpp/RawFileClass.h"
#include <cerrno>
#include <cstring>

RawFileClass::RawFileClass(noinit_t tag) : FileClass(tag) {}

RawFileClass::~RawFileClass() {
    RawFileClass::Close();
    if (FileNameAllocated && FileName) {
        game::FreeName(const_cast<char*>(FileName));
        FileName = nullptr;
        FileNameAllocated = false;
    }
}

void RawFileClass::CDCheck(DWORD , bool , char const * )
{
}

RawFileClass::RawFileClass(char const * filename) :
    FileAccess(FileAccessMode::None),
    FilePointer(0),
    FileSize(-1),
    Handle(RAW_NULL_HANDLE),
    FileName(filename),
    unknown_short_1C(0),
    unknown_short_1E(0),
    FileNameAllocated(false),
    padding_21{}
{
}

char const * RawFileClass::SetFileName(char const * filename)
RA2_FILE_TRY {
#ifdef RA2_FILES_GAME
    if (FileName && FileNameAllocated) {
        game::FreeName(const_cast<char*>(FileName));
        FileName = nullptr;
        FileNameAllocated = false;
    }
    if (!filename) return nullptr;
    Bias(0);
    FileName = game::DuplicateName(filename);
    FileNameAllocated = FileName != nullptr;
    if (!FileName) this->CDCheck(ENOMEM, false, filename);
    return FileName;
#else
    char* copy = filename ? game::DuplicateName(filename) : nullptr;
    if (filename && !copy) { this->CDCheck(ENOMEM, false, filename); return(nullptr); }
    if (FileNameAllocated) game::FreeName(const_cast<char*>(FileName));
    FileName = copy;
    FileNameAllocated = copy != nullptr;
    if (filename) Bias(0);

    return(FileName);
#endif
} RA2_FILE_FAILURE(nullptr)

bool RawFileClass::OpenEx(char const * filename, FileAccessMode rights)
RA2_FILE_TRY {
    if (!this->SetFileName(filename)) return(false);
    return(this->Open(rights));
} RA2_FILE_FAILURE(0)

bool RawFileClass::Open(FileAccessMode rights) RA2_FILE_TRY {
    this->Close();
    if (!FileName) { this->CDCheck(ENOENT, false, nullptr); return false; }
    FileAccess = rights;
    if (rights != FileAccessMode::Read && rights != FileAccessMode::Write && rights != FileAccessMode::ReadWrite) {
        this->CDCheck(EINVAL, false, FileName); return false;
    }
    const auto creation = rights == FileAccessMode::Read ? game::FileCreation::existing
        : rights == FileAccessMode::Write ? game::FileCreation::truncate : game::FileCreation::open_or_create;
    const auto result = game::current_file_system().open(FileName, static_cast<game::FileMode>(rights),
        creation, Handle, rights == FileAccessMode::Read ? 3 : 0, rights == FileAccessMode::Read);
    if (!result) { this->CDCheck(result.error, false, FileName); return false; }
    if (FilePointer || FileSize != -1) {
        if (this->Seek(0, FileSeekMode::Set) < 0) { this->Close(); return false; }
    }
    return true;
} RA2_FILE_FAILURE(0)

bool RawFileClass::Exists(bool forced) RA2_FILE_TRY {
    if (!FileName) return false;
    if (this->HasHandle()) return true;
    if (forced) {
        const bool opened = RawFileClass::Open(FileAccessMode::Read);
        RawFileClass::Close();
#ifdef RA2_FILES_GAME
        return true; // 65CBF0 returns AL=1 even after failed forced Open.
#else
        return opened; // Independent caller protection, explicitly not original failure semantics.
#endif
    }
    if (!game::current_file_system().open(FileName, game::FileMode::read,
        game::FileCreation::existing, Handle, 1)) return false;
    const auto result = game::current_file_system().close(Handle);
    if (!result) this->CDCheck(result.error, false, FileName);
    Handle = RAW_NULL_HANDLE;
    return true;
} RA2_FILE_FAILURE(0)

void RawFileClass::Close() RA2_FILE_TRY {
    if (this->HasHandle()) {
        const auto result = game::current_file_system().close(Handle);
        if (!result) this->CDCheck(result.error, false, FileName);
        Handle = RAW_NULL_HANDLE;
    }
} RA2_FILE_FAILURE()

int32_t RawFileClass::ReadBytes(void * buffer, int32_t size)
RA2_FILE_TRY {
#ifndef RA2_FILES_GAME
    if (!buffer || size <= 0) return(0);
#endif
    int32_t	bytesread = 0;
    int	opened = false;

    if (!this->HasHandle()) {

        if (!this->Open(FileAccessMode::Read)) {
            return(0);
        }
        opened = true;
    }

    if (FileSize != -1) {
        const int32_t position = this->Seek(0, FileSeekMode::Current);
        if (position < 0) { if (opened) this->Close(); return(0); }
        int remainder = FileSize - position;
        size = size < remainder ? size : remainder;
    }

    while (size > 0) {
        std::size_t actual = 0;
        const auto result = game::current_file_system().read(Handle, buffer, size, actual);
        buffer = static_cast<char*>(buffer) + actual;
        bytesread += int32_t(actual);
        size -= int32_t(actual);
        if (!result) {
#ifdef RA2_FILES_GAME
            if (game::file_read_aborted()) break;
            this->CDCheck(result.error, true, FileName);
            continue;
#else
            this->CDCheck(result.error, true, FileName); break;
#endif
        }
        if (!actual) break;
    }


    game::clear_file_read_error();
    if (opened) this->Close();
    return(bytesread);
} RA2_FILE_FAILURE(0)

int32_t RawFileClass::WriteBytes(void * buffer, int32_t size)
RA2_FILE_TRY {
#ifndef RA2_FILES_GAME
    if (!buffer || size <= 0) return(0);
#endif
    int32_t	bytesread = 0;
    int	opened = false;

    if (!this->HasHandle()) {
        if (!this->Open(FileAccessMode::Write)) {
            return(0);
        }
        opened = true;
    }

    std::size_t actual = 0;
    const auto result = game::current_file_system().write(Handle, buffer, size, actual);
    bytesread = int32_t(actual);
    if (!result) this->CDCheck(result.error, false, FileName);


    if (FileSize != -1) {
        if (RawSeek(0) > FilePointer+FileSize) {
            FileSize = RawSeek(0) - FilePointer;
        }
    }

    if (opened) {
        this->Close();
    }

    return(bytesread);
} RA2_FILE_FAILURE(0)

int RawFileClass::Seek(int pos, FileSeekMode dir)
RA2_FILE_TRY {

    if (FileSize != -1) {
        switch (dir) {
            case FileSeekMode::Set:
                if (pos > FileSize) {
                    pos = FileSize;
                }
                pos += FilePointer;
                break;

            case FileSeekMode::Current:
                break;

            case FileSeekMode::End:
                dir = FileSeekMode::Set;
                pos += FilePointer + FileSize;
                break;
        }

        const int32_t physical = RawSeek(pos, dir);
        if (physical < 0) return(-1);
        int32_t newpos = physical - FilePointer;

        if (newpos < 0) {
            newpos = RawSeek(FilePointer, FileSeekMode::Set) - FilePointer;
        }
        if (newpos > FileSize) {
            newpos = RawSeek(FilePointer+FileSize, FileSeekMode::Set) - FilePointer;
        }
        return(newpos);
    }

    return(RawSeek(pos, dir));
} RA2_FILE_FAILURE(0)

int32_t RawFileClass::GetFileSize(void)
RA2_FILE_TRY {
    int32_t	size = 0;

    if (FileSize != -1) {
        return(FileSize);
    }

    if (this->HasHandle()) {

        std::uint64_t physical = 0;
        const auto result = game::current_file_system().size(Handle, physical);
        if (!result || physical > INT32_MAX) {
            this->CDCheck(result ? EOVERFLOW : result.error, false, FileName);
            return -1;
        }
        size = int32_t(physical);


    } else {

        if (this->Open(FileAccessMode::Read)) {
            size = this->GetFileSize();

            this->Close();
        }
#ifndef RA2_FILES_GAME
        else { return(-1); }
#endif
    }

    if (size < 0) return(-1);
    FileSize = size-FilePointer;
    return(FileSize);
} RA2_FILE_FAILURE(0)

BOOL RawFileClass::CreateFile(void)
RA2_FILE_TRY {
    this->Close();
    if (this->Open(FileAccessMode::Write)) {

        if (FileSize != -1) {
            this->Seek(0, FileSeekMode::Set);
        }

        this->Close();
        return(true);
    }
    return(false);
} RA2_FILE_FAILURE(0)

BOOL RawFileClass::DeleteFile(void)
RA2_FILE_TRY {
    this->Close();

    if (!FileName) {
        this->CDCheck(ENOENT, false, nullptr);
        return(false);
    }

    for (;;) {

        if (!this->Exists(false)) {
            return(false);
        }

        const auto result = game::current_file_system().remove(FileName);
        if (!result) { this->CDCheck(result.error, false, FileName); return false; }

        break;
    }

    return(true);
} RA2_FILE_FAILURE(0)

DWORD RawFileClass::GetFileTime() RA2_FILE_TRY {
    std::uint32_t packed = 0;
    game::current_file_system().get_time(Handle, packed);
    return packed;
} RA2_FILE_FAILURE(0)
bool RawFileClass::SetFileTime(DWORD packed) RA2_FILE_TRY {
    return bool(game::current_file_system().set_time(Handle, packed));
} RA2_FILE_FAILURE(0)

void RawFileClass::Bias(int start, int length)
RA2_FILE_TRY {
    if (start == 0) {
        FilePointer = 0;
        FileSize = -1;
        return;
    }

    FileSize = RawFileClass::GetFileSize();
    FilePointer += start;
    if (length != -1) {
        FileSize = FileSize < length ? FileSize : length;
    }
    FileSize = FileSize > 0 ? FileSize : 0;

    if (this->HasHandle()) {
        RawFileClass::Seek(0, FileSeekMode::Set);
    }
} RA2_FILE_FAILURE()

int RawFileClass::RawSeek(int pos, FileSeekMode dir) {
#ifdef RA2_FILES_GAME
    constexpr int failure_position = 0;
#else
    constexpr int failure_position = -1;
#endif
    if (!this->HasHandle()) { this->CDCheck(EBADF, false, FileName); return failure_position; }
    std::uint64_t position = 0;
    const auto result = game::current_file_system().seek(Handle, pos, static_cast<game::FileOrigin>(dir), position);
    if (!result || position > INT32_MAX) {
        this->CDCheck(result ? EOVERFLOW : result.error, false, FileName); return failure_position;
    }
    return int32_t(position);
}

// Non-template interface helpers; bodies retained from the corresponding header.

const char* RawFileClass::GetFileName() const
{ return FileName; }

bool RawFileClass::HasHandle()
{ return Handle != RAW_NULL_HANDLE; }

RawFileClass::RawFileClass()
    : FileAccess(FileAccessMode::Read), FilePointer(0), FileSize(-1),
      Handle(RAW_NULL_HANDLE), FileName(nullptr), unknown_short_1C(0), unknown_short_1E(0), FileNameAllocated(false), padding_21{}
{}
