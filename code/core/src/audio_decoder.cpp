#include "audio_decoder.hpp"
#include "yrpp/RawFileClass.h"
#include <algorithm>
#include <array>
#include <bit>

namespace game {
namespace {
constexpr int steps[] = {7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,   21,    23,
                         25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,   73,    80,
                         88,    97,    107,   118,   130,   143,   157,   173,   190,   209,   230,  253,   279,
                         307,   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,  876,   963,
                         1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749, 3024,  3327,
                         3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487,
                         12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
constexpr int indices[] = {-1, -1, -1, -1, 2, 4, 6, 8};
int signed16(const unsigned char* p) {
    return std::bit_cast<std::int16_t>(std::uint16_t(p[0] | (unsigned(p[1]) << 8)));
}
int nibble(unsigned value, int& predictor, int& index) {
    // 0x0040ACD0 rounds each term separately, unlike multiply-based decoders.
    const int step = steps[index];
    int delta = step >> 3;
    if (value & 1) delta += step >> 2;
    if (value & 2) delta += step >> 1;
    if (value & 4) delta += step;
    predictor = std::clamp(predictor + ((value & 8) ? -delta : delta), -32768, 32767);
    index = std::clamp(index + indices[value & 7], 0, 88);
    return predictor;
}
}
bool AudioDecoder::open(RawFileClass& file, const AudioSampleData& info, int bytes) noexcept {
    file_ = nullptr;
    remaining_ = 0;
    block_.clear();
    cursor_ = 0;
    if (bytes <= 0 || info.SampleRate < 1000 || info.SampleRate > 192000 || info.NumChannels < 1 ||
        info.NumChannels > 2 || info.Format > 1)
        return false;
    const unsigned channels = info.NumChannels;
    if (info.Format == 0) {
        if ((info.BytesPerSample != 1 && info.BytesPerSample != 2) ||
            unsigned(bytes) % (channels * info.BytesPerSample))
            return false;
    } else if (info.BlockAlign < channels * 4 || info.BlockAlign > 65536 ||
               (info.BlockAlign - channels * 4) % (channels * 4))
        return false;
    try {
        encoded_.reserve(info.Format ? info.BlockAlign : 16384);
        block_.reserve(info.Format ? 1 + (info.BlockAlign - channels * 4) * 2 / channels : 8192);
        info_ = info;
        remaining_ = unsigned(bytes);
        file_ = &file;
        return true;
    } catch (...) {
        return false;
    }
}
bool AudioDecoder::next_block() {
    block_.clear();
    cursor_ = 0;
    if (!remaining_) return true;
    unsigned bytes = std::min(remaining_, info_.Format ? info_.BlockAlign : 16384u);
    const unsigned channels = info_.NumChannels;
    if (!info_.Format) bytes -= bytes % (channels * info_.BytesPerSample);
    if (!bytes) return false;
    encoded_.resize(bytes);
    if (file_->ReadBytes(encoded_.data(), int(bytes)) != int(bytes)) return false;
    remaining_ -= bytes;
    if (!info_.Format) {
        const unsigned width = info_.BytesPerSample;
        block_.resize(bytes / (channels * width));
        const auto sample = [width](const unsigned char* p) {
            return width == 1 ? (int(*p) - 128) / 128.0f : signed16(p) / 32768.0f;
        };
        for (unsigned i = 0; i < block_.size(); ++i) {
            const auto* p = encoded_.data() + i * channels * width;
            const float left = sample(p);
            block_[i] = {left, channels == 2 ? sample(p + width) : left};
        }
        return true;
    }
    // Microsoft IMA WAV: one predictor/index header per channel, followed by
    // alternating four-byte groups per channel (low nibble first).
    if (bytes < 4 * channels) return false;
    int predictor[2]{}, index[2]{};
    for (unsigned c = 0; c < channels; ++c) {
        const auto* p = encoded_.data() + c * 4;
        predictor[c] = signed16(p);
        index[c] = p[2];
        if (index[c] > 88 || p[3]) return false;
    }
    // BAG entries can finish with an incomplete stereo channel group. Consume
    // the entry but emit only complete paired groups; never invent the missing
    // channel. Mono tails can use every remaining encoded byte.
    const unsigned payload = bytes - 4 * channels;
    const unsigned usable = channels == 1 ? payload : payload - payload % 8;
    block_.resize(1 + usable * 2 / channels);
    block_[0] = {predictor[0] / 32768.0f, predictor[channels - 1] / 32768.0f};
    unsigned frame = 1;
    for (unsigned offset = channels * 4; offset < channels * 4 + usable; offset += channels * 4, frame += 8) {
        const unsigned samples = std::min(8u, (channels * 4 + usable - offset) * 2 / channels);
        for (unsigned c = 0; c < channels; ++c) {
            for (unsigned i = 0; i < samples; ++i) {
                const unsigned b = encoded_[offset + c * 4 + i / 2];
                const float value = nibble((b >> ((i & 1) * 4)) & 15, predictor[c], index[c]) / 32768.0f;
                if (!c) block_[frame + i].left = value;
                if (c || channels == 1) block_[frame + i].right = value;
            }
        }
    }
    return true;
}
bool AudioDecoder::read(AudioPCMFrame* output, unsigned capacity, unsigned& frames) noexcept {
    if (!file_ || (!output && capacity)) return false;
    unsigned written = 0;
    try {
        while (written < capacity) {
            if (cursor_ == block_.size()) {
                if (!remaining_) break;
                if (!next_block()) return false;
            }
            const auto count = std::min<std::size_t>(capacity - written, block_.size() - cursor_);
            std::copy_n(block_.data() + cursor_, count, output + written);
            written += unsigned(count);
            cursor_ += count;
        }
        frames = written;
        return true;
    } catch (...) {
        return false;
    }
}
}
