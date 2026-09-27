// IDX layout/entry access adapted from XCC misc/audio_idx_file.{h,cpp} and
// misc/cc_structures.h, revision 70358b46858973426c1ecf204485cb2a88716217.
// Copyright (C) 2000 Olaf van der Spek <olafvdspek@gmail.com>
// GPL-3.0-or-later: redistribute/modify under GNU GPL version 3 or later.
// Distributed WITHOUT ANY WARRANTY, including MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See third_party/xcc/COPYING for the license.
// Original AudioIDXData ownership/operations calibrated to gamemd 0x4011C0..0x401938.
#include "yrpp/Audio.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/Memory.h"
#include <bit>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#ifndef _WIN32
#include <glob.h>
#include <sys/stat.h>
#endif

namespace {
std::uint32_t read32(const unsigned char* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
bool valid_index(const AudioIDXData& library, int index) {
    return library.Samples && index >= 0 && index < library.SampleCount;
}
int YRPP_CDECL compare_name(const void* a, const void* b) {
    // Name is the first member: usable by both qsort(entries) and bsearch(name).
    return _strcmpi(static_cast<const char*>(a), static_cast<const char*>(b));
}
bool directory_found(const char* pattern) {
#ifdef _WIN32
    WIN32_FIND_DATAA entry{};
    const auto handle = FindFirstFileA(pattern, &entry);
    if (handle == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    do { if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { found = true; break; } }
    while (FindNextFileA(handle, &entry));
    FindClose(handle);
    return found;
#else
    std::string path(pattern);
    for (auto& c : path) if (c == '\\') c = '/';
    glob_t matches{};
    bool found = false;
    if (::glob(path.c_str(), GLOB_NOSORT, nullptr, &matches) == 0) {
        for (std::size_t i = 0; i < matches.gl_pathc; ++i) {
            struct stat info{};
            if (::stat(matches.gl_pathv[i], &info) == 0 && S_ISDIR(info.st_mode)) { found = true; break; }
        }
    }
    ::globfree(&matches);
    return found;
#endif
}
}

#ifndef RA2_AUDIO_GAME
namespace { AudioIDXData* audio_index = nullptr; }
AudioIDXData*& AudioIDXData::Instance = audio_index;
#endif

AudioIDXData::AudioIDXData()
    : Samples(nullptr), SampleCount(0), Path{}, BagFile(nullptr), ExternalFile(nullptr),
      PathFound(0), CurrentSampleFile(nullptr), CurrentSampleSize(0), unknown_120(0) {}

AudioIDXData::~AudioIDXData() {
    delete BagFile;
    delete ExternalFile;
    YRMemory::Deallocate(Samples);
}

AudioIDXData* YRPP_FASTCALL AudioIDXData::Create(const char* filename, const char* path) {
    if (!filename || !*filename) return nullptr;
    try {
        std::string stem(filename);
        // Target strips the final dot, even when it belongs to a directory.
        if (const auto dot = stem.rfind('.'); dot != std::string::npos) stem.resize(dot);
        if (stem.size() + 4 >= MAX_PATH || (path && std::strlen(path) + 1 >= MAX_PATH)) return nullptr;
        const std::string idx_name = stem + ".idx", bag_name = stem + ".bag";
        std::unique_ptr<AudioIDXData> library(new (std::nothrow) AudioIDXData);
        if (!library) return nullptr;
        CCFileClass idx(idx_name.c_str());
        if (!idx.Exists()) return nullptr;
        library->BagFile = new (std::nothrow) CCFileClass(bag_name.c_str());
        if (!library->BagFile || !library->BagFile->Exists()) return nullptr;
        // 0x401300 ignores the Open results; the IDX read decides success.
        idx.Open(FileAccessMode::Read);
        library->BagFile->Open(FileAccessMode::Read);
        unsigned char header[12]{};
        if (idx.ReadBytes(header, 12) != 12) return nullptr;
        const auto count = read32(header + 8);
        if (count > std::uint32_t(INT_MAX / sizeof(AudioIDXEntry))) return nullptr;
        library->SampleCount = int(count);
        const int stride = read32(header + 4) == 1 ? 32 : 36;
        // Match target version dispatch; it does not validate the magic.
        if (count) {
            library->Samples = static_cast<AudioIDXEntry*>(YRMemory::AllocateBytes(count * sizeof(AudioIDXEntry)));
            if (!library->Samples) return nullptr;
            for (std::uint32_t i = 0; i < count; ++i) {
                unsigned char entry[36]{};
                if (idx.ReadBytes(entry, stride) != stride || !std::memchr(entry, 0, 16)) return nullptr;
                auto& sample = library->Samples[i];
                std::memcpy(sample.Name, entry, 16);
                sample.Offset = std::bit_cast<std::int32_t>(read32(entry + 16));
                sample.Size = std::bit_cast<std::int32_t>(read32(entry + 20));
                sample.SampleRate = read32(entry + 24);
                sample.Flags = read32(entry + 28);
                sample.ChunkSize = read32(entry + 32); // Version 1 tail stays zero.
            }
            std::qsort(library->Samples, count, sizeof(AudioIDXEntry), compare_name);
        }
        if (path) {
            const auto length = std::strlen(path);
            std::memcpy(library->Path, path, length + 1);
            library->PathFound = directory_found(path) ? 1 : 0;
            // Target tests the terminating NUL against '\\', so always appends.
            library->Path[length] = '\\';
            library->Path[length + 1] = 0;
        }
        return library.release();
    } catch (...) { return nullptr; }
}

void AudioIDXData::ClearCurrentSample() {
    CurrentSampleFile = nullptr;
    delete ExternalFile;
    ExternalFile = nullptr;
}

int YRPP_FASTCALL AudioIDXData::FindSampleIndex(const char* name) const {
    if (!name || !Samples || SampleCount <= 0) return -1;
    const auto* entry = static_cast<const AudioIDXEntry*>(
        std::bsearch(name, Samples, std::size_t(SampleCount), sizeof(AudioIDXEntry), compare_name));
    return entry ? int(entry - Samples) : -1;
}
const char* YRPP_FASTCALL AudioIDXData::GetSampleName(int index) const {
    return valid_index(*this, index) ? Samples[index].Name : "Invalid"; // Original 0x815E88.
}
int YRPP_FASTCALL AudioIDXData::GetSampleSize(int index) const {
    return valid_index(*this, index) ? Samples[index].Size : 0;
}
AudioSampleData* YRPP_FASTCALL AudioIDXData::GetSampleInformation(int index, AudioSampleData* output) const {
    if (!output || !valid_index(*this, index)) return nullptr;
    const auto& entry = Samples[index];
    output->Data = 4;
    output->SampleRate = entry.SampleRate;
    output->NumChannels = (entry.Flags & 1) + 1;
    output->BlockAlign = entry.ChunkSize;
    output->Format = (entry.Flags & 8) ? 1 : 0;
    output->BytesPerSample = (entry.Flags & 8) ? 2 : ((entry.Flags & 4) ? 2 : 1);
    // Target leaves ByteRate and Flags untouched.
    return output;
}

bool YRPP_FASTCALL AudioIDXData::OpenSample(int index) {
    if (!valid_index(*this, index) || !BagFile) return false;
    ClearCurrentSample();
    auto& entry = Samples[index];
    CurrentSampleFile = BagFile;
    CurrentSampleSize = entry.Size;
    const bool bag_ready = BagFile->Seek(entry.Offset, FileSeekMode::Set) == entry.Offset && entry.Size != 0;
    if (!PathFound) return bag_ready;
    const std::string name = std::string(Path) + entry.Name + ".wav";
    if (name.size() >= MAX_PATH) return bag_ready;
    std::unique_ptr<RawFileClass> external(new (std::nothrow) RawFileClass);
    if (!external || !external->SetFileName(name.c_str()) || !external->Exists() ||
        !external->Open(FileAccessMode::Read)) return bag_ready;
    AudioSampleData info;
    int data_size = 0;
    if (!Audio::ReadWAVFile(external.get(), &info, &data_size)) return bag_ready;
    entry.Flags = info.BytesPerSample == 2 ? 6 : 2;
    if (info.NumChannels == 2) entry.Flags |= 1;
    entry.SampleRate = info.SampleRate;
    CurrentSampleSize = data_size;
    ExternalFile = external.release();
    CurrentSampleFile = ExternalFile;
    return true;
}
