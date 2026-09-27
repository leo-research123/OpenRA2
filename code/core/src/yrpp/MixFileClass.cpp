// Copyright 2020 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from REDALERT/MIXFILE.CPP; see third_party/ea/README.md and LICENSE.TXT.
#include "yrpp/MixFileClass.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/CCFileClass.h"
#include "filesystem/file_system.hpp"
#include "filesystem/file_names.hpp"
#ifndef RA2_FILES_GAME
#include "filesystem/resource_context.hpp"
#endif
#include "yrpp/CRC.h"
#include "third_party/xcc/blowfish.hpp"

#include <array>
#include <vector>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>
#include <stdexcept>

#if !defined(RA2_YRPP_GAME) && !defined(RA2_FILES_GAME) && !defined(RA2_IMAGE_GAME)
// Host-only storage for MixFileClass.h. Original aliases remain in compat.
namespace {
List<MixFileClass> mixes;
DynamicVectorClass<MixFileClass*> expansions;
DynamicVectorClass<MixFileClass*> map_archives;
MixFileClass* multiplayer_archive=nullptr;
MixFileClass* sidebar_noncached=nullptr;
MixFileClass::GenericMixFiles generics{};
}
List<MixFileClass>& MixFileClass::MIXes = mixes;
DynamicVectorClass<MixFileClass*>& MixFileClass::Array = expansions;
MixFileClass::GenericMixFiles& MixFileClass::Generics = generics;
DynamicVectorClass<MixFileClass*>& MixFileClass::Maps = map_archives;
MixFileClass*& MixFileClass::MULTIMD = multiplayer_archive;
MixFileClass*& MixFileClass::SIDENC = sidebar_noncached;
#endif

namespace {
uint16_t le16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
// MIX parsing owns its input bounds; no second file object or cursor is kept.
bool read_index_bytes(CCFileClass& file, uint64_t length, size_t count,
    std::vector<uint8_t>& output) {
    if (!file.HasHandle() || count > size_t(INT32_MAX)) return false;
    const int position = file.Seek(0, FileSeekMode::Current);
    if (position < 0 || uint64_t(position) > length || count > length - uint64_t(position)) return false;
    std::vector<uint8_t> bytes(count);
    if (count && file.ReadBytes(bytes.data(), int(count)) != int(count)) return false;
    output = std::move(bytes);
    return true;
}
bool seek_index(CCFileClass& file, uint64_t length, uint64_t position) {
    return file.HasHandle() && position <= length && position <= uint64_t(INT32_MAX) &&
        file.Seek(int(position), FileSeekMode::Set) == int(position);
}
bool read_index(MixFileClass& mix, CCFileClass& file, uint64_t length, const PKey* public_key) {
    std::vector<uint8_t> prefix;
    if (!read_index_bytes(file, length, 4, prefix)) throw std::runtime_error("truncated MIX prefix");
    const bool old_format = le16(prefix.data()) != 0;
    const auto flags = old_format ? 0u : le32(prefix.data());
    mix.IsDigest = (flags & 0x10000) != 0;
    mix.IsEncrypted = (flags & 0x20000) != 0;
    ra2::xcc::Cblowfish cipher;
    std::vector<uint8_t> header;
    uint64_t index_start = old_format ? 0 : 4;
    const bool decrypt = mix.IsEncrypted && public_key;
    if (decrypt) {
        const int blocks = public_key->Block_Count(56);
        if (!blocks) throw std::runtime_error("invalid MIX public key precision");
        const int encrypted_size = blocks * public_key->Crypt_Block_Size();
        std::vector<uint8_t> source;
        if (!read_index_bytes(file, length, encrypted_size, source)) throw std::runtime_error("truncated MIX key");
        std::array<uint8_t, 256> decoded{};
        if (public_key->Decrypt(source.data(), encrypted_size, decoded.data()) < 56)
            throw std::runtime_error("cannot recover MIX key");
        cipher.set_key(std::span<const uint8_t, 56>(decoded.data(), 56));
        index_start = 4 + uint64_t(encrypted_size);
        if (!read_index_bytes(file, length, 8, header)) throw std::runtime_error("truncated encrypted MIX header");
        cipher.decipher(header.data(), header.data(), 8);
    } else {
        if (!seek_index(file, length, index_start)) throw std::runtime_error("MIX index offset is outside the file");
        if (!read_index_bytes(file, length, 6, header)) throw std::runtime_error("truncated MIX header");
    }
    const auto count = le16(header.data());
    // The original sign-extends CountFiles from a 16-bit field (5B3D75).
    if (count > 32767) throw std::runtime_error("negative MIX entry count");
    const auto data_size = le32(header.data() + 2);
    if (data_size > uint32_t(INT32_MAX)) throw std::runtime_error("MIX data size exceeds signed 32-bit range");
    mix.CountFiles = int32_t(count);
    mix.FileSize = int32_t(data_size);
    const size_t index_size = 6 + size_t(count) * 12;
    const size_t stored_size = decrypt ? (index_size + 7) & ~size_t(7) : index_size;
    if (!seek_index(file, length, index_start)) throw std::runtime_error("MIX index offset is outside the file");
    std::vector<uint8_t> index;
    if (!read_index_bytes(file, length, stored_size, index)) throw std::runtime_error("truncated MIX index");
    if (decrypt) cipher.decipher(index.data(), index.data(), int(index.size()));
    const auto relative_start = index_start + stored_size;
    if (relative_start > length || data_size > length - relative_start)
        throw std::runtime_error("MIX data exceeds containing file");
    // Construction does not verify the optional SHA-1 trailer; Cache owns that step.
    // Original 5B3DCC..5B3DDF: CCFile's carrier bias plus its current cursor.
    // This also works when the carrier is a borrowed in-memory MIX cache.
    const auto data_start = uint64_t(uint32_t(file.FilePointer)) + relative_start;
    if (data_start > uint32_t(INT32_MAX)) throw std::runtime_error("MIX data start exceeds signed 32-bit range");
    mix.Headers = static_cast<MixHeaderData*>(YRMemory::Allocate(sizeof(MixHeaderData) * count));
    if (!mix.Headers) return false;
    for (size_t i = 0; i < count; ++i) {
        const auto p = index.data() + 6 + 12 * i;
        const MixHeaderData entry{le32(p), le32(p + 4), le32(p + 8)};
        if (entry.Offset > data_size || entry.Size > data_size - entry.Offset)
            throw std::runtime_error("MIX member exceeds data region");
        new (mix.Headers + i) MixHeaderData(entry);
    }
    mix.FileStartOffset = int32_t(data_start);
    // Do not sort or deduplicate: 5B4430 searches the stored signed-ID order.
    return true;
}
}

const MixHeaderData* MixFileClass::find(uint32_t id) const {
    if (!Headers) return nullptr;
    size_t start = 0, count = CountFiles > 0 ? size_t(CountFiles) : 0;
    const auto target = std::bit_cast<int32_t>(id);
    while (count) {
        const auto half = count / 2;
        const auto& entry = Headers[start + half];
        const auto candidate = std::bit_cast<int32_t>(entry.ID);
        if (candidate == target) return &entry;
        if (candidate > target) count = half;
        else { start += half + 1; count -= half + 1; }
    }
    return nullptr;
}

// The filename-only convenience uses the standard public key. The original
// two-argument entry always honors its supplied key, including a null key.
MixFileClass::MixFileClass(const char* filename) : MixFileClass(filename, &DefaultKey()) {}
MixFileClass::MixFileClass(const char* filename, const PKey* key) {
    if (!filename) return;
    try {
        game::prepare_mix_file();
#ifndef RA2_FILES_GAME
        // Source diagnostics are optional and never determine the archive bias.
        auto* parent = game::try_current_context();
        if (parent && parent->source) *parent->source = {};
        game::FileLocation location;
        game::ResourceScope scope(location);
#endif
        CCFileClass file(filename);
        // 5B3C9D duplicates the temporary file's name BEFORE opening it; Open
        // may replace that name with the containing archive's name.
        FileName = game::DuplicateName(file.GetFileName());
        if (!file.Open(FileAccessMode::Read)) return;
        const int length = file.GetFileSize();
        if (length < 0) throw std::runtime_error("cannot determine MIX file size");
#ifndef RA2_FILES_GAME
        if (parent && parent->source) *parent->source = location;
#endif
        if (!read_index(*this, file, uint64_t(length), key)) return;
        MIXes.AddTail(this);
    } catch (...) {
        YRMemory::Deallocate(Headers);
        Headers = nullptr;
#ifdef RA2_FILES_GAME
        // Malformed input cannot propagate a C++ exception into the EXE. Leave
        // an unregistered, destructible object; missing files take the same path
        // without throwing. Never install an EXE vtable on this core object.
        CountFiles = 0;
        FileStartOffset = 0;
#else
        game::FreeName(const_cast<char*>(FileName));
        FileName = nullptr;
        throw;
#endif
    }
}

MixFileClass::~MixFileClass() {
    if (FileName) game::FreeName(const_cast<char*>(FileName));
    if (Data && IsAllocated) {
        delete[] static_cast<char*>(Data);
        IsAllocated = false;
    }
    Data = nullptr;
    if (Headers) {
        YRMemory::Deallocate(Headers);
        Headers = nullptr;
    }
    Unlink();
}

bool YRPP_FASTCALL MixFileClass::Offset(const char* filename, void** realptr,
    MixFileClass** mixfile, int* offset, int* size) {
    if (!filename) return false;
    // RA2/YR CRC and signed binary lookup replace the older RA hash.
    CRCEngine crc;
    for (const char* p = filename; *p; ++p) {
        char c = *p;
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        crc(c);
    }
    const auto id = uint32_t(crc());
    for (auto* mix : MIXes) {
        const auto* block = mix->find(id);
        if (!block) continue;
        if (mixfile) *mixfile = mix;
        if (size) *size = std::bit_cast<int32_t>(block->Size);
        if (realptr) *realptr = mix->Data ? static_cast<char*>(mix->Data) + block->Offset : nullptr;
        {
            // Match the original ADD's low 32 bits without signed-overflow UB.
            const auto start = mix->Data ? 0u : uint32_t(mix->FileStartOffset);
            if (offset) *offset = std::bit_cast<int32_t>(block->Offset + start);
        }
        return true;
    }
    // Original and EA implementation both leave all outputs untouched on miss.
    return false;
}

#ifndef RA2_FILES_GAME
bool YRPP_FASTCALL MixFileClass::Cache(const char*, const void*) { return true; }
#endif

bool MixFileClass::headers(const MixHeaderData*& entries, int& count) const noexcept {
    const auto* available = Headers;
    const int available_count = CountFiles;
    const bool present = available && available_count > 0;
    entries = present ? available : nullptr;
    count = present ? available_count : 0;
    return present;
}
