/*
    RIFF/WAVE chunk traversal adapted from XCC misc/wav_file.cpp and
    misc/wav_structures.h at 70358b46858973426c1ecf204485cb2a88716217.
    Copyright (C) 2000 Olaf van der Spek <olafvdspek@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
    The repository retains the license in third_party/xcc/COPYING.
*/
// Calibrated to gamemd 0x408610.
#include "yrpp/Audio.h"
#include "yrpp/AudioHelpers.hpp"
#include "api/audio_backend.hpp"
#include "yrpp/RawFileClass.h"
#include "yrpp/Memory.h"
#include <algorithm>
#include <bit>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>

namespace game {
void YRPP_FASTCALL audio_stop_playback_4025b0(void* playback) noexcept {
    AudioBackend* backend = nullptr;
    if (get_audio_backend(backend)) backend->stop_playback(static_cast<AudioPlaybackHandle>(playback));
}
void YRPP_FASTCALL audio_release_buffer_408e70(void* buffer) noexcept {
    AudioBackend* backend = nullptr;
    if (get_audio_backend(backend)) backend->release_buffer(static_cast<AudioBufferHandle>(buffer));
}
}

namespace {
std::uint16_t read16(const unsigned char* p) {
    return std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8);
}
std::uint32_t read32(const unsigned char* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
struct FreeFormat {
    void operator()(unsigned char* p) const noexcept { YRMemory::Deallocate(p); }
};
}

bool YRPP_FASTCALL Audio::ReadWAVFile(RawFileClass* file, AudioSampleData* sample, int* data_size) {
    if (data_size) *data_size = 0;
    // The original dereferences both outputs once it reaches a data chunk.
    if (!file || !sample || !data_size) return false;
    file->Seek(0, FileSeekMode::Set);
    unsigned char header[12]{};
    if (file->ReadBytes(header, 12) != 12 || std::memcmp(header, "RIFF", 4) ||
        std::memcmp(header + 8, "WAVE", 4) || !file->HasHandle()) return false;

    std::unique_ptr<unsigned char, FreeFormat> format;
    unsigned char chunk[8]{};
    while (file->ReadBytes(chunk, 8) == 8) {
        const auto size = read32(chunk + 4);
        if (!std::memcmp(chunk, "fmt ", 4)) {
            // Preserve the target's zero-filled short fmt payload. Its file
            // API takes an int; reject unrepresentable allocation/read sizes.
            if (size > INT_MAX) return false;
            const auto capacity = std::max<std::size_t>(18, size);
            format.reset(static_cast<unsigned char*>(YRMemory::AllocateBytes(capacity)));
            if (!format) return false;
            std::memset(format.get(), 0, capacity);
            file->ReadBytes(format.get(), int(size));
            // Original overwrites cbSize with the fmt chunk length.
            format.get()[16] = static_cast<unsigned char>(size);
            format.get()[17] = static_cast<unsigned char>(size >> 8);
        } else if (!std::memcmp(chunk, "data", 4)) {
            *data_size = std::bit_cast<std::int32_t>(size);
            if (!format) return false;
            const auto* fmt = format.get();
            switch (read16(fmt)) {
            case 0x11: // WAV IMA ADPCM -> original internal format 1
                sample->Format = 1;
                sample->BlockAlign = read16(fmt + 12);
                sample->BytesPerSample = 2;
                break;
            case 1: // PCM leaves the caller's BlockAlign untouched.
                sample->Format = 0;
                sample->BytesPerSample = read16(fmt + 14) >> 3;
                break;
            default:
                return false;
            }
            sample->Data = 4;
            sample->NumChannels = read16(fmt + 2);
            sample->SampleRate = read32(fmt + 4);
            const std::uint32_t rate = sample->SampleRate * sample->NumChannels * sample->BytesPerSample;
            // 0x4087C0 uses a signed shift after the original 32-bit multiply.
            sample->ByteRate = sample->Format == 1
                ? std::uint32_t(std::bit_cast<std::int32_t>(rate) >> 2) : rate;
            // Flags is also preserved. The data bytes are consumed by callers.
            return true;
        } else {
            // The target skips exactly size bytes, including odd-sized chunks;
            // it neither rounds to RIFF alignment nor enforces the RIFF length.
            file->Seek(std::bit_cast<std::int32_t>(size), FileSeekMode::Current);
        }
        if (!file->HasHandle()) break;
    }
    return false;
}
