#include <gtest/gtest.h>
#include "audio_mixer.hpp"
#include "audio_session.hpp"
#include <cmath>
#include <memory>
#include <vector>

namespace {
using namespace game;
std::shared_ptr<AudioSource> sample(unsigned rate, std::initializer_list<float> values) {
    auto result = std::make_shared<AudioSource>();
    result->rate = rate;
    for (float value : values)
        result->pcm.push_back({value, value});
    return result;
}
void start(AudioMixer& mixer, AudioSource& source, float pitch = 1, bool loop = false) {
    std::uint64_t command = 0;
    ASSERT_TRUE(mixer.submit({AudioMixOperation::start, 0, 1, &source, 1, 0.5f, pitch, loop}, command));
}
std::vector<float> mix(AudioMixer& mixer, unsigned frames) {
    std::vector<float> result(frames * 2);
    mixer.mix(result.data(), frames);
    return result;
}
void expect_mono(const std::vector<float>& output, std::initializer_list<float> expected) {
    ASSERT_EQ(output.size(), expected.size() * 2);
    unsigned i = 0;
    for (float value : expected) {
        EXPECT_NEAR(output[i++], value, 0.00001f);
        EXPECT_NEAR(output[i++], value, 0.00001f);
    }
}
}

class AudioMixerTest : public testing::Test {};
class AudioSessionTest : public testing::Test {};

TEST_F(AudioMixerTest, StreamRingWrapRefillAndFinalNeighbor) {
    AudioSource source;
    source.rate = 24000;
    source.streaming = true;
    source.pcm.resize(audio_ring_capacity);
    for (unsigned i = 0; i < audio_ring_capacity; ++i)
        source.pcm[i] = {0.2f, 0.2f};
    source.pcm.back() = {0.4f, 0.4f};
    source.written.store(audio_ring_capacity);
    AudioMixer mixer(48000);
    start(mixer, source);
    mix(mixer, (audio_ring_capacity - 1) * 2);
    EXPECT_EQ(source.read.load(), audio_ring_capacity - 1);
    expect_mono(mix(mixer, 2), {0, 0});
    source.pcm[0] = {0.6f, 0.6f};
    source.written.store(audio_ring_capacity + 1, std::memory_order_release);
    source.eof.store(true, std::memory_order_release);
    expect_mono(mix(mixer, 5), {0.4f, 0.5f, 0.6f, 0.6f, 0});
    EXPECT_EQ(source.read.load(), audio_ring_capacity + 1);
    EXPECT_EQ(mixer.finished(0), 1u);
}

TEST_F(AudioMixerTest, UnitStepLoopAndGainRamp) {
    auto source = sample(48000, {0.5f, 0.5f, 0.5f});
    AudioMixer mixer(48000);
    start(mixer, *source, 1, true);
    expect_mono(mix(mixer, 4), {0.5f, 0.5f, 0.5f, 0.5f});
    std::uint64_t command = 0;
    ASSERT_TRUE(mixer.submit({AudioMixOperation::parameters, 0, 1, nullptr, 0, 0.5f, 1, true}, command));
    expect_mono(mix(mixer, 4), {0.375f, 0.25f, 0.125f, 0});
    EXPECT_EQ(mixer.finished(0), 0u);
}

TEST_F(AudioMixerTest, NativeRateCompletesOneSecondOfAudioAfter22050Frames) {
    auto source = sample(22050, {0.25f});
    source->pcm.resize(22050, {0.25f, 0.25f});
    AudioMixer mixer(22050);
    start(mixer, *source);
    const auto output = mix(mixer, 22049);
    EXPECT_FLOAT_EQ(output.back(), 0.25f);
    EXPECT_EQ(mixer.finished(0), 0u);
    expect_mono(mix(mixer, 1), {0.25f});
    EXPECT_EQ(mixer.finished(0), 1u);
}

TEST_F(AudioSessionTest, HandlesRejectNullForeignMisalignedAndDeadSlots) {
    auto session = std::make_unique<AudioSession>(48000);
    EXPECT_EQ(session->event(static_cast<AudioEventHandle>(nullptr)), nullptr);
    AudioEventTag foreign;
    EXPECT_EQ(session->event(&foreign), nullptr);
    EXPECT_EQ(session->event(session->events.data() + session->events.size()), nullptr);
    EXPECT_EQ(session->event(
                  reinterpret_cast<AudioEventHandle>(reinterpret_cast<std::uintptr_t>(session->events.data()) - 1)),
              nullptr);
    for (auto& entry : session->events) {
        EXPECT_EQ(session->event(&entry), nullptr);
        entry.alive = true;
        EXPECT_EQ(session->event(&entry), &entry);
        EXPECT_EQ(session->event(reinterpret_cast<AudioEventHandle>(reinterpret_cast<std::uintptr_t>(&entry) + 1)),
                  nullptr);
    }
}
