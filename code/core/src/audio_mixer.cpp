#include "audio_mixer.hpp"
#include <algorithm>
#include <cmath>

namespace game {
namespace {
// Original 0x816380 table: hundredths of a decibel, indexed by integer percent.
constexpr int device_attenuation[] = {
    -10000, -6644, -5644, -5059, -4644, -4322, -4059, -3837, -3644, -3474, -3322, -3184, -3059, -2943, -2837,
    -2737,  -2644, -2556, -2474, -2396, -2322, -2252, -2184, -2120, -2059, -2000, -1943, -1889, -1837, -1786,
    -1737,  -1690, -1644, -1599, -1556, -1515, -1474, -1434, -1396, -1358, -1322, -1286, -1252, -1218, -1184,
    -1152,  -1120, -1089, -1059, -1029, -1000, -971,  -943,  -916,  -889,  -862,  -837,  -811,  -786,  -761,
    -737,   -713,  -690,  -667,  -644,  -621,  -599,  -578,  -556,  -535,  -515,  -494,  -474,  -454,  -434,
    -415,   -396,  -377,  -358,  -340,  -322,  -304,  -286,  -269,  -252,  -234,  -218,  -201,  -184,  -168,
    -152,   -136,  -120,  -105,  -89,   -74,   -59,   -44,   -29,   -14,   0};
float amplitude(int attenuation) noexcept {
    // DirectSound's minimum-volume and full-pan endpoints are silence.
    return attenuation <= -10000 ? 0.0f : std::pow(10.0f, float(attenuation) / 2000.0f);
}
}
int audio_device_volume_db(unsigned volume) noexcept {
    return device_attenuation[(100u * std::min(volume, 0x4000u)) >> 14];
}
int audio_device_pan_db(unsigned pan) noexcept {
    const int offset = int((200u * std::min(pan, 0x4000u)) >> 14) - 100;
    return offset < 0 ? device_attenuation[100 + offset] : -device_attenuation[100 - offset];
}
void audio_device_gains(float volume, float pan, float& left, float& right) noexcept {
    const unsigned level = unsigned(std::clamp(volume, 0.0f, 1.0f) * 0x4000);
    const unsigned balance = unsigned(std::clamp(pan, 0.0f, 1.0f) * 0x4000);
    const float gain = amplitude(audio_device_volume_db(level));
    const int panning = audio_device_pan_db(balance);
    left = gain * amplitude(panning > 0 ? -panning : 0);
    right = gain * amplitude(panning < 0 ? panning : 0);
}
bool AudioSource::fill() noexcept {
    if (sequence) return sequence->fill();
    if (!streaming || eof.load(std::memory_order_relaxed)) return true;
    auto end = written.load(std::memory_order_relaxed);
    const auto begin = read.load(std::memory_order_acquire);
    unsigned free = audio_ring_capacity - unsigned(end - begin);
    while (free) {
        const unsigned offset = unsigned(end % audio_ring_capacity);
        const unsigned capacity = std::min(free, audio_ring_capacity - offset);
        unsigned count = 0;
        if (!decoder.read(pcm.data() + offset, capacity, count)) return false;
        end += count;
        free -= count;
        written.store(end, std::memory_order_release);
        if (decoder.finished()) {
            eof.store(true, std::memory_order_release);
            break;
        }
        if (!count) return false;
    }
    return true;
}
bool AudioMixer::submit(const AudioMixCommand& command, std::uint64_t& sequence) noexcept {
    const auto tail = tail_.load(std::memory_order_relaxed);
    if (tail - head_.load(std::memory_order_acquire) >= capacity) return false;
    commands_[tail % capacity] = command;
    tail_.store(tail + 1, std::memory_order_release);
    sequence = tail + 1;
    return true;
}
void AudioMixer::mix(float* output, unsigned frames) noexcept {
    if (!output) return;
    std::fill_n(output, std::size_t(frames) * 2, 0.0f);
    auto head = head_.load(std::memory_order_relaxed);
    const auto tail = tail_.load(std::memory_order_acquire);
    while (head != tail) {
        const auto command = commands_[head % capacity];
        if (command.operation == AudioMixOperation::suspend)
            suspended_ = command.flag;
        else if (command.slot < voices_.size()) {
            auto& voice = voices_[command.slot];
            if (command.operation == AudioMixOperation::start) {
                voice = {};
                voice.source = command.source;
                voice.stamp = command.stamp;
                voice.gain = command.gain;
                voice.pan = command.pan;
                voice.pitch = command.pitch;
                audio_device_gains(voice.gain, voice.pan, voice.left, voice.right);
                voice.loop = command.flag;
            } else if (voice.stamp == command.stamp) {
                if (command.operation == AudioMixOperation::stop) {
                    voice.source = nullptr;
                    finished_[command.slot].store(voice.stamp, std::memory_order_release);
                } else if (command.operation == AudioMixOperation::pause)
                    voice.paused = command.flag;
                else {
                    voice.gain = command.gain;
                    voice.pan = command.pan;
                    voice.pitch = command.pitch;
                    voice.loop = command.flag;
                }
            }
        }
        head_.store(++head, std::memory_order_release);
    }
    if (suspended_) return;
    if (!frames) return;
    for (unsigned slot = 0; slot < voices_.size(); ++slot) {
        auto& voice = voices_[slot];
        auto* source = voice.source;
        if (!source || voice.paused) continue;
        auto* playlist = source->sequence.get();
        float target_left = 0, target_right = 0;
        audio_device_gains(voice.gain, voice.pan, target_left, target_right);
        float left = voice.left, right = voice.right;
        const float left_delta = (target_left - left) / frames;
        const float right_delta = (target_right - right) / frames;
        // EOF must be observed before the boundary it publishes. Snapshot once
        // per callback; a concurrent refill becomes visible on the next block.
        const bool eof = playlist ? playlist->eof.load(std::memory_order_acquire)
                                  : !source->streaming || source->eof.load(std::memory_order_acquire);
        const auto end = playlist            ? playlist->written.load(std::memory_order_acquire)
                         : source->streaming ? source->written.load(std::memory_order_acquire)
                                             : source->pcm.size();
        unsigned mixed = 0;
        while (true) {
            const AudioSource* part = source;
            if (playlist) {
                if (voice.part >= end) {
                    if (eof)
                        voice.source = nullptr;
                    else
                        underruns_.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
                part = playlist->queue[voice.part % AudioEventPlaylist::capacity].source;
            }
            const auto length = playlist ? part->pcm.size() : end;
            if (voice.cursor >= length) {
                if (playlist && voice.part + 1 < end) {
                    const auto* next = playlist->queue[(voice.part + 1) % AudioEventPlaylist::capacity].source;
                    // Carry time, not source-frame units, across rate changes.
                    const double position = (double(voice.cursor - length) + voice.fraction) * next->rate / part->rate;
                    voice.cursor = static_cast<std::uint64_t>(position);
                    voice.fraction = position - double(voice.cursor);
                    ++voice.part;
                    continue;
                }
                if (!playlist && voice.loop && !source->streaming && length) {
                    voice.cursor %= length;
                    continue;
                }
                if (eof) {
                    if (playlist) voice.part = end;
                    voice.source = nullptr;
                } else if (mixed < frames)
                    underruns_.fetch_add(1, std::memory_order_relaxed);
                break;
            }
            if (mixed == frames) break;
            const double step = double(part->rate) * voice.pitch / rate_;
            if (!(step > 0) || !std::isfinite(step)) break;
            const auto size = part->pcm.size();
            const auto offset = source->streaming ? voice.cursor % size : voice.cursor;
            const auto* data = part->pcm.data() + offset;
            const auto available = std::min<std::uint64_t>(length - voice.cursor, size - offset);
            // Interior spans have an adjacent interpolation neighbor. Ring
            // wraps, playlist transitions and EOF are handled outside the loop.
            const bool boundary = available == 1;
            const AudioPCMFrame* neighbor = nullptr;
            if (boundary) {
                if (voice.cursor + 1 < length)
                    neighbor = part->pcm.data();
                else if (playlist && voice.part + 1 < end)
                    neighbor = playlist->queue[(voice.part + 1) % AudioEventPlaylist::capacity].source->pcm.data();
                else if (!playlist && voice.loop && !source->streaming)
                    neighbor = part->pcm.data();
                else if (!eof) {
                    underruns_.fetch_add(1, std::memory_order_relaxed);
                    break;
                } else
                    neighbor = data;
            }
            const double limit = boundary ? 1.0 : double(available - 1);
            double position = voice.fraction;
            auto* destination = output + 2 * mixed;
            unsigned count = frames - mixed;
            const double possible = std::ceil((limit - position) / step);
            if (possible < count) count = unsigned(possible);
            unsigned done = 0;
            if (!boundary && step == 1.0 && position == 0.0) {
                if (left_delta == 0 && right_delta == 0) {
                    for (; done < count; ++done) {
                        destination[2 * done] += data[done].left * left;
                        destination[2 * done + 1] += data[done].right * right;
                    }
                } else {
                    for (; done < count; ++done) {
                        left += left_delta;
                        right += right_delta;
                        destination[2 * done] += data[done].left * left;
                        destination[2 * done + 1] += data[done].right * right;
                    }
                }
                position += done;
            } else if (boundary) {
                for (; done < count && position < limit; ++done) {
                    left += left_delta;
                    right += right_delta;
                    const float fraction = float(position);
                    destination[2 * done] += (data->left + (neighbor->left - data->left) * fraction) * left;
                    destination[2 * done + 1] += (data->right + (neighbor->right - data->right) * fraction) * right;
                    position += step;
                }
            } else {
                for (; done < count && position < limit; ++done) {
                    left += left_delta;
                    right += right_delta;
                    const auto index = static_cast<std::uint64_t>(position);
                    const float fraction = float(position - double(index));
                    const auto* a = data + index;
                    const auto* b = a + 1;
                    destination[2 * done] += (a->left + (b->left - a->left) * fraction) * left;
                    destination[2 * done + 1] += (a->right + (b->right - a->right) * fraction) * right;
                    position += step;
                }
            }
            const auto advance = static_cast<std::uint64_t>(position);
            voice.cursor += advance;
            voice.fraction = position - double(advance);
            mixed += done;
            if (source->streaming && voice.cursor > end) voice.cursor = end;
        }
        voice.left = target_left;
        voice.right = target_right;
        if (playlist)
            playlist->consumed.store(voice.part, std::memory_order_release);
        else if (source->streaming)
            source->read.store(voice.cursor, std::memory_order_release);
        // This is the last access to source storage before publishing completion.
        if (!voice.source) finished_[slot].store(voice.stamp, std::memory_order_release);
    }
    for (std::size_t i = 0; i < std::size_t(frames) * 2; ++i) {
        const float value = output[i];
        output[i] = value < -1.0f ? -1.0f : value > 1.0f ? 1.0f : value;
    }
}
}
