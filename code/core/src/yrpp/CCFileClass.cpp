#include "filesystem/file_system.hpp"
// Copyright 2020 Electronic Arts Inc. See third_party/ea/LICENSE.TXT.
// Adapted from REDALERT/CCFILE.CPP at f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// YRpp interfaces and target 4739F0..473FC0; host bounds/recursion guards are explicit.
#include "yrpp/CCFileClass.h"
#include "yrpp/MixFileClass.h"
#ifndef RA2_FILES_GAME
#include "filesystem/resource_context.hpp"
#include "filesystem/resource_environment.hpp"
#endif
#include <algorithm>
#include <cstring>
#include <string>
#include <stdexcept>

namespace {
thread_local unsigned open_depth = 0;
struct OpenDepth {
    OpenDepth() { if (++open_depth > 128) { --open_depth; throw std::runtime_error("cyclic MIX carrier lookup"); } }
    ~OpenDepth() { --open_depth; }
};
void observe_disk(CCFileClass& file) {
#ifndef RA2_FILES_GAME
    if (auto* context = game::try_current_context(); context && context->source) {
        auto& source = *context->source;
        source = {};
        game::current_file_system().resolve(file.GetFileName(), false, source.physical_file);
        source.size = file.GetFileSize();
        source.source_chain.emplace_back(file.GetFileName());
    }
#else
    (void)file;
#endif
}
}
CCFileClass::CCFileClass(const char* name) { CDFileClass::SetFileName(name); }
CCFileClass::~CCFileClass() { Position = 0; }
const char* CCFileClass::SetFileName(const char* name) RA2_FILE_TRY { Availablility = 0; return CDFileClass::SetFileName(name); } RA2_FILE_FAILURE(nullptr)
bool CCFileClass::Exists(bool) RA2_FILE_TRY {
    if (this->HasHandle()) return true;
    if (FileName && MixFileClass::Offset(FileName, nullptr, nullptr, nullptr, nullptr)) return true;
    return CDFileClass::Exists(false);
} RA2_FILE_FAILURE(0)
bool CCFileClass::HasHandle() RA2_FILE_TRY { return Buffer.Buffer || BufferIOFileClass::HasHandle(); } RA2_FILE_FAILURE(0)
bool CCFileClass::OpenEx(const char* name, FileAccessMode mode) RA2_FILE_TRY {
    this->SetFileName(name);
    return this->Open(mode);
} RA2_FILE_FAILURE(0)
bool CCFileClass::Open(FileAccessMode mode) RA2_FILE_TRY {
    OpenDepth depth;
    this->Close();
    if (!FileName) return false;
    if ((unsigned(mode) & 2) || CDFileClass::Exists(false)) {
        const bool ok = CDFileClass::Open(mode);
        if (ok) observe_disk(*this);
        return ok;
    }
    MixFileClass* mix = nullptr;
    void* data = nullptr;
    int start = 0, length = 0;
    if (!MixFileClass::Offset(FileName, &data, &mix, &start, &length)) {
        const bool ok = CDFileClass::Open(mode);
        if (ok) observe_disk(*this);
        return ok;
    }
    if (data) {
        Buffer.Buffer = data; Buffer.Size = length; Buffer.Allocated = false;
        Position = 0;
#ifndef RA2_FILES_GAME
        if (auto* c = game::try_current_context(); c && c->source)
            *c->source = {{}, 0, std::uint32_t(length), {mix->FileName, FileName}, static_cast<const uint8_t*>(data)};
#endif
        return true;
    }
    if (!mix || !mix->FileName) return false;
    const std::string member(FileName);
    if (!this->OpenEx(mix->FileName, FileAccessMode::Read)) {
#ifndef RA2_FILES_GAME
        throw std::runtime_error("MIX carrier is unavailable");
#else
        return false;
#endif
    }
    // A nested member can live in a borrowed cached carrier. Keep that view
    // bounded while interpreting Offset as the original carrier coordinate.
    if (Buffer.Buffer) {
        const auto relative = std::int64_t(start) - FilePointer;
        if (relative < 0 || length < 0 || relative + length > Buffer.Size)
            throw std::runtime_error("MIX view exceeds cached carrier");
        Buffer.Buffer = static_cast<BYTE*>(Buffer.Buffer) + relative;
        Buffer.Size = length;
        Position = 0;
    } else {
#ifndef RA2_FILES_GAME
        const auto carrier_size = RawFileClass::GetFileSize();
        if (carrier_size < 0 || start < FilePointer || length < 0
            || std::int64_t(start) + length > std::int64_t(FilePointer) + carrier_size)
            throw std::runtime_error("file view exceeds physical file");
#endif
    }
    IsDisabled = true;
    // 473DBF obtains the name after recursive OpenEx: it is the carrier name.
    // Keep the requested member separately for host source diagnostics only.
    const std::string carrier_name(this->GetFileName());
    this->SetFileName(carrier_name.c_str());
    IsDisabled = false;
    Bias(0);
    if (!Buffer.Buffer) { Bias(start, length); FileSize = length; this->Seek(0, FileSeekMode::Set); }
    else { FilePointer = start; FileSize = length; }
#ifndef RA2_FILES_GAME
    if (auto* c = game::try_current_context(); c && c->source) {
        c->source->offset = start; c->source->size = length;
        c->source->source_chain.push_back(member);
        if (Buffer.Buffer) { c->source->memory = static_cast<const uint8_t*>(Buffer.Buffer); c->source->offset = 0; }
    }
#endif
    return true;
} RA2_FILE_FAILURE(0)
int CCFileClass::ReadBytes(void* destination, int size) RA2_FILE_TRY {
#ifndef RA2_FILES_GAME
    if (size < 0 || (!destination && size > 0)) return 0;
#endif
    const bool opened = !this->HasHandle();
    if (opened && !this->Open(FileAccessMode::Read)) return 0;
    int count;
    if (Buffer.Buffer) {
        count = std::min(size, Buffer.Size - int(Position));
        if (count > 0) std::memmove(destination, static_cast<const BYTE*>(Buffer.Buffer) + Position, count);
        Position += count;
    } else count = CDFileClass::ReadBytes(destination, size);
    if (opened) this->Close();
    return count;
} RA2_FILE_FAILURE(0)
int CCFileClass::Seek(int offset, FileSeekMode mode) RA2_FILE_TRY {
    if (!Buffer.Buffer) return CDFileClass::Seek(offset, mode);
    if (mode == FileSeekMode::Set) Position = 0;
    else if (mode == FileSeekMode::End) Position = Buffer.Size;
    Position = std::clamp<std::int64_t>(std::int64_t(Position) + offset, 0, Buffer.Size);
    return int(Position);
} RA2_FILE_FAILURE(0)
int CCFileClass::GetFileSize() RA2_FILE_TRY {
    if (Buffer.Buffer) return Buffer.Size;
    if (BufferIOFileClass::Exists(false)) return BufferIOFileClass::GetFileSize();
    int size = 0;
    if (FileName) MixFileClass::Offset(FileName, nullptr, nullptr, nullptr, &size);
    return size;
} RA2_FILE_FAILURE(0)
int CCFileClass::WriteBytes(void* buffer, int size) RA2_FILE_TRY {
    if (Buffer.Buffer) { this->CDCheck(13, false, FileName); return 0; }
    return CDFileClass::WriteBytes(buffer, size);
} RA2_FILE_FAILURE(0)
void CCFileClass::Close() RA2_FILE_TRY { Buffer.Clear(); Position = 0; CDFileClass::Close(); } RA2_FILE_FAILURE()
DWORD CCFileClass::GetFileTime() RA2_FILE_TRY {
    const auto time = CDFileClass::GetFileTime();
    if (time) return time;
    MixFileClass* mix = nullptr;
    if (!FileName || !MixFileClass::Offset(FileName, nullptr, &mix, nullptr, nullptr) || !mix) return 0;
    CCFileClass carrier(mix->FileName);
    return carrier.GetFileTime();
} RA2_FILE_FAILURE(0)
bool CCFileClass::SetFileTime(DWORD time) RA2_FILE_TRY {
    if (CDFileClass::SetFileTime(time)) return true;
    MixFileClass* mix = nullptr;
    if (!FileName || !MixFileClass::Offset(FileName, nullptr, &mix, nullptr, nullptr) || !mix) return false;
    CCFileClass carrier(mix->FileName);
    return carrier.SetFileTime(time);
} RA2_FILE_FAILURE(0)
void CCFileClass::CDCheck(DWORD code, bool retry, const char* name) { RawFileClass::CDCheck(code, retry, name); }
