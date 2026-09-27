#include "support/test_support.hpp"
#include "api/audio_backend.hpp"
#include "yrpp/Audio.h"
#include "yrpp/AudioHelpers.hpp" // Explicit internal dependency in Tests.cmake.
#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>

static_assert(!std::is_convertible_v<game::AudioPlaybackHandle, game::AudioEventHandle>);
static_assert(!std::is_convertible_v<game::AudioBufferHandle, game::AudioEventHandle>);
static_assert(!std::is_convertible_v<game::AudioBufferHandle, game::AudioPlaybackHandle>);

namespace {

enum Operation { play, destroy, stop, end, stop_loop, end_loop, bind, event_query,
    controller_type_query, volume, panning, event_type_query, stop_playback, release_buffer, count };
class Probe final : public game::AudioBackend {
public:
    std::array<int, count> calls{};
    void* object = nullptr;
    AudioController* controller = nullptr;
    const char* filename = nullptr;
    bool resource_file = false;
    bool play_result = true;
    bool alive = true;
    unsigned int value = 0;
    int event_storage = 0;
    int type_storage = 0;
    int* destructions = nullptr;

    ~Probe() override { if (destructions) ++*destructions; }
    game::AudioEventHandle event() noexcept { return reinterpret_cast<game::AudioEventHandle>(&event_storage); }
    VocClass* type() noexcept { return reinterpret_cast<VocClass*>(&type_storage); }
    void record(Operation operation, void* target) noexcept { ++calls[operation]; object = target; }

    bool play_wav(AudioStream& stream, const char* name, bool resource) noexcept override {
        record(play, &stream); filename = name; resource_file = resource; return play_result;
    }
    void destroy_controller(AudioController& target) noexcept override {
        record(destroy, &target); target.Event = nullptr; target.EventType = nullptr;
    }
    void stop(AudioController& target) noexcept override { record(::stop, &target); }
    void end(AudioController& target) noexcept override { record(::end, &target); }
    void stop_looping(AudioController& target) noexcept override { record(stop_loop, &target); }
    void end_looping(AudioController& target) noexcept override { record(end_loop, &target); }
    void set_event(AudioController& target, game::AudioEventHandle handle, VocClass* event_type) noexcept override {
        record(bind, handle); controller = &target;
        target.Event = handle; target.EventType = event_type; ++target.Stamp;
    }
    bool get_event(AudioController& target, game::AudioEventHandle& output) noexcept override {
        record(event_query, &target);
        if (!alive || !target.Event) return false;
        output = static_cast<game::AudioEventHandle>(target.Event); return true;
    }
    bool get_controller_type(AudioController& target, VocClass*& output) noexcept override {
        record(controller_type_query, &target);
        if (!alive || !target.EventType) return false;
        output = target.EventType; return true;
    }
    void set_volume(game::AudioEventHandle handle, unsigned int volume) noexcept override {
        record(::volume, handle); value = volume;
    }
    void set_panning(game::AudioEventHandle handle, unsigned int pan) noexcept override {
        record(panning, handle); value = pan;
    }
    bool get_event_type(game::AudioEventHandle handle, VocClass*& output) noexcept override {
        record(event_type_query, handle);
        if (!alive || handle != event()) return false;
        output = type(); return true;
    }
    void stop_playback(game::AudioPlaybackHandle playback) noexcept override { record(::stop_playback, playback); }
    void release_buffer(game::AudioBufferHandle buffer) noexcept override { record(::release_buffer, buffer); }
};
void contracts() {
    game::reset_audio_backend();
    Probe probe;
    game::AudioBackend* sentinel = &probe;
    EXPECT_TRUE((!game::get_audio_backend(sentinel) && sentinel == &probe)) << "unavailable backend preserves output";
    {
        // This empty stream is used only with a fixture backend which treats
        // it as an identity. It must never be passed to the original backend.
        AudioStream stream;
        AudioController controller;
        EXPECT_TRUE((!stream.PlayWAV("missing.wav", false))) << "unbound playback reports failure";
        EXPECT_TRUE((!controller.GetEvent() && !controller.GetEventType() &&
            !AudioController::GetEventType(probe.event()))) << "unbound queries report misses";
        controller.Stop(); controller.End(); controller.StopLooping(); controller.EndLooping();
        controller.SetEvent(probe.event(), probe.type());
        AudioController::AdjustAudioEventVolume(probe.event(), 1);
        AudioController::AdjustAudioEventPanning(probe.event(), 1);
        game::audio_stop_playback_4025b0(probe.event());
        game::audio_release_buffer_408e70(probe.event());
    }

    game::set_audio_backend(probe);
    struct Reset { ~Reset() { game::reset_audio_backend(); } } reset;
    game::AudioBackend* installed = nullptr;
    EXPECT_TRUE((game::get_audio_backend(installed) && installed == &probe)) << "registration borrows the host object without copying its state";
    int destructions = 0;
    {
        Probe replacement;
        replacement.destructions = &destructions;
        game::set_audio_backend(replacement);
        EXPECT_TRUE((game::get_audio_backend(installed) && installed == &replacement)) << "backend replacement";
        game::reset_audio_backend();
        EXPECT_TRUE((destructions == 0 && !game::get_audio_backend(installed) && installed == &replacement)) << "reset neither deletes the host object nor overwrites failed query output";
    }
    EXPECT_TRUE((destructions == 1)) << "host retains ownership of backend destruction";
    game::set_audio_backend(probe);
    EXPECT_TRUE((game::get_audio_backend(installed) && installed == &probe)) << "backend reinstallation";

    {
        AudioStream stream;
        const char filename[] = "sound.wav";
        EXPECT_TRUE((stream.PlayWAV(filename, true))) << "host playback success propagated";
        EXPECT_TRUE((probe.object == &stream && probe.filename == filename && probe.resource_file)) << "stream identity, filename and lookup flag reach host";
        probe.play_result = false;
        EXPECT_TRUE((!stream.PlayWAV(filename, false) && !probe.resource_file)) << "host playback failure and raw-file flag propagated";

        AudioController controller;
        controller.Unused = 0xabcdef01u;
        auto* index = controller.AudioIndex;
        controller.SetEvent(probe.event(), probe.type());
        EXPECT_TRUE((probe.controller == &controller && probe.object == probe.event() && controller.Stamp == 1)) << "event binding and backend state updates reach original controller";
        EXPECT_TRUE((controller.GetEvent() == probe.event())) << "controller event query";
        EXPECT_TRUE((controller.GetEventType() == probe.type())) << "controller type query";
        EXPECT_TRUE((AudioController::GetEventType(probe.event()) == probe.type())) << "static event type query";

        controller.Stop(); EXPECT_TRUE((probe.calls[stop] == 1 && probe.object == &controller)) << "stop dispatch";
        controller.End(); EXPECT_TRUE((probe.calls[end] == 1 && probe.object == &controller)) << "fade dispatch";
        controller.StopLooping(); EXPECT_TRUE((probe.calls[stop_loop] == 1)) << "stop loop dispatch";
        controller.EndLooping(); EXPECT_TRUE((probe.calls[end_loop] == 1)) << "end loop dispatch";
        AudioController::AdjustAudioEventVolume(probe.event(), 0xfedcba98u);
        EXPECT_TRUE((probe.calls[volume] == 1 && probe.value == 0xfedcba98u && probe.object == probe.event())) << "volume retains all 32 bits and event identity";
        AudioController::AdjustAudioEventPanning(probe.event(), 0x87654321u);
        EXPECT_TRUE((probe.calls[panning] == 1 && probe.value == 0x87654321u && probe.object == probe.event())) << "panning retains all 32 bits and event identity";

        probe.alive = false;
        game::AudioEventHandle missing_event = probe.event();
        VocClass* missing_type = probe.type();
        EXPECT_TRUE((!installed->get_event(controller, missing_event) && missing_event == probe.event())) << "miss leaves event output unchanged";
        EXPECT_TRUE((!installed->get_controller_type(controller, missing_type) &&
            missing_type == probe.type())) << "miss leaves type output unchanged";
        EXPECT_TRUE((!controller.GetEvent() && !controller.GetEventType() &&
            !AudioController::GetEventType(probe.event()))) << "backend misses become YRpp null results";
        EXPECT_TRUE((controller.AudioIndex == index && controller.Unused == 0xabcdef01u)) << "dispatch adds no state changes to unrelated original fields";

        game::audio_stop_playback_4025b0(&stream);
        EXPECT_TRUE((probe.calls[stop_playback] == 1 && probe.object == &stream)) << "opaque playback dispatch";
        game::audio_release_buffer_408e70(probe.event());
        EXPECT_TRUE((probe.calls[release_buffer] == 1 && probe.object == probe.event())) << "opaque buffer dispatch";
    }
    EXPECT_TRUE((probe.calls[destroy] == 1)) << "controller destruction reaches host before unregister";
    for (int calls : probe.calls) EXPECT_TRUE((calls > 0)) << "every audio backend operation exercised";
}
}

TEST(AudioBackend, Contracts) {
    contracts();
    game::AudioBackend* backend = nullptr;
    EXPECT_TRUE((!game::get_audio_backend(backend))) << "reset restores standalone unavailable default";
}
