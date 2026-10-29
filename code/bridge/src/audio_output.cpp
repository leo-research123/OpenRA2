#include "bridge/audio_output.hpp"
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/audio_frame.hpp>
#include <algorithm>
#include <array>
#include <atomic>

struct RA2AudioLink {
    std::atomic<game::AudioSession*> session{nullptr};
    std::atomic<game::MovieSession*> movies{nullptr};
};
void RA2AudioPlayback::_start(double) {
    playing_ = true;
    // Reset interpolation history on every start, including a reused playback.
    begin_resample();
}
void RA2AudioPlayback::_stop() { playing_ = false; }
bool RA2AudioPlayback::_is_playing() const { return playing_; }
float RA2AudioPlayback::_get_stream_sampling_rate() const { return float(RA2AudioOutput::sample_rate); }
int32_t RA2AudioPlayback::_mix_resampled(godot::AudioFrame* buffer, int32_t frames) {
    if (!buffer || frames <= 0) return 0;
    auto* session = link_ ? link_->session.load(std::memory_order_acquire) : nullptr;
    if (!playing_ || !session) {
        std::fill_n(buffer, frames, godot::AudioFrame{});
        return frames;
    }
    // Do not assume AudioFrame has the core's interleaved-float ABI.
    auto* movies = link_->movies.load(std::memory_order_acquire);
    std::array<float, 2048> pcm, media;
    for (int32_t offset = 0; offset < frames;) {
        const int32_t count = std::min(1024, frames - offset);
        game::mix_audio_session(*session, pcm.data(), unsigned(count));
        if (movies)
            game::mix_movie_session(*movies, media.data(), unsigned(count));
        else
            media.fill(0);
        for (int32_t i = 0; i < count; ++i) {
            buffer[offset + i].left = pcm[2 * i] + media[2 * i];
            buffer[offset + i].right = pcm[2 * i + 1] + media[2 * i + 1];
        }
        offset += count;
    }
    return frames;
}
godot::Ref<godot::AudioStreamPlayback> RA2AudioStream::_instantiate_playback() const {
    godot::Ref<RA2AudioPlayback> playback;
    playback.instantiate();
    playback->link_ = link_;
    return playback;
}
bool RA2AudioOutput::open(game::AudioSession& session, game::MovieSession* movies) noexcept {
    try {
        auto* tree = godot::Object::cast_to<godot::SceneTree>(godot::Engine::get_singleton()->get_main_loop());
        if (!tree || !tree->get_root()) return false;
        stream_.instantiate();
        stream_->link_ = std::make_shared<RA2AudioLink>();
        stream_->link_->session.store(&session, std::memory_order_release);
        stream_->link_->movies.store(movies, std::memory_order_release);
        auto* player = memnew(godot::AudioStreamPlayer);
        player_id_ = player->get_instance_id();
        player->set_name("RA2AudioOutput");
        player->set_process_mode(godot::Node::PROCESS_MODE_ALWAYS);
        tree->get_root()->add_child(player);
        player->set_stream(stream_);
        player->play();
        return true;
    } catch (...) {
        return false;
    }
}
std::uint64_t RA2AudioOutput::playback_id() const noexcept {
    auto* player = godot::Object::cast_to<godot::AudioStreamPlayer>(godot::ObjectDB::get_instance(player_id_));
    if (!player || !player->has_stream_playback()) return 0;
    const auto playback = player->get_stream_playback();
    return playback.is_valid() ? std::uint64_t(playback->get_instance_id()) : 0;
}
RA2AudioOutput::~RA2AudioOutput() {
    if (stream_.is_valid() && stream_->link_) {
        // AudioServer's mixer lock also drains an in-flight _mix before the
        // session is detached. No core memory is reclaimed in the callback.
        auto* server = godot::AudioServer::get_singleton();
        server->lock();
        stream_->link_->session.store(nullptr, std::memory_order_release);
        stream_->link_->movies.store(nullptr, std::memory_order_release);
        server->unlock();
    }
    // SceneTree may destroy its child before the last RA2Core reference.
    // Resolve the instance ID without touching a potentially freed wrapper.
    if (auto* player = godot::Object::cast_to<godot::AudioStreamPlayer>(godot::ObjectDB::get_instance(player_id_))) {
        player->stop();
        // EXIT_TREE can run while the parent is traversing its children.
        // The detached session is already safe; let the tree retire the node.
        if (player->get_parent())
            player->queue_free();
        else
            memdelete(player);
    }
}
