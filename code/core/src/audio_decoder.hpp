#pragma once
#include "yrpp/Audio.h"
#include <cstdint>
#include <vector>

namespace game {
struct AudioPCMFrame {
    float left = 0, right = 0;
};

// Decoder borrows the file at its payload position. A library sample must be
// consumed before AudioIDXData opens another sample on its shared BAG cursor.
class AudioDecoder {
public:
    bool open(RawFileClass&, const AudioSampleData&, int bytes) noexcept;
    bool read(AudioPCMFrame* output, unsigned capacity, unsigned& frames) noexcept;
    bool finished() const noexcept { return remaining_ == 0 && cursor_ == block_.size(); }
    unsigned rate() const noexcept { return info_.SampleRate; }

private:
    bool next_block();
    RawFileClass* file_ = nullptr;
    AudioSampleData info_;
    unsigned remaining_ = 0;
    std::vector<unsigned char> encoded_;
    std::vector<AudioPCMFrame> block_;
    std::size_t cursor_ = 0;
};
}
