#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "api/filesystem.hpp"
#include "api/audio_backend.hpp"
#include "api/movie_decoder.hpp"
#include "api/map_view.hpp"
#include "api/presentation.hpp"
#include "api/map_objects.hpp"
#include "api/debug_commands.hpp"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include "bridge/ra2_ini.hpp"
#include <mutex>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class RA2AudioOutput;

class RA2Core : public godot::RefCounted {
    GDCLASS(RA2Core, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    RA2Core();
    ~RA2Core() override;
    godot::Dictionary get_audio_status() const;
    godot::Dictionary get_audio_volumes() const noexcept;
    bool set_audio_volumes(double effects, double speech, double music) noexcept;
    void set_activity(bool focused, bool paused) noexcept;
    bool set_game_speed_level(std::int64_t level) noexcept;
    godot::Array get_hotkeys() noexcept;
    bool set_hotkeys(const godot::Dictionary& bindings) noexcept;
    std::int64_t encode_hotkey(const godot::Ref<godot::InputEvent>& event) noexcept;
    godot::String get_version() const;
    void begin_resource_loading(const godot::String& directory);
    godot::Dictionary get_resource_progress() const;
    void cancel_resource_loading();
    godot::Ref<RA2INI> load_ini(const godot::String& filename);
    // Owner-thread lookup. Returns false for missing/unavailable text; success
    // writes UTF-16-decoded text to the caller's shared Godot dictionary.
    bool try_get_localized_string(const godot::String& label, godot::Dictionary output) noexcept;
    // Owner-thread, idle-map operation. Mounts the original map packages for
    // menu discovery; failures return false and no exception crosses Godot.
    bool prepare_map_resources() noexcept;
    void begin_map_loading(const godot::String& filename);
    void begin_campaign_loading(const godot::String& filename, int difficulty);
    void begin_skirmish_loading(const godot::String& filename, const godot::Dictionary& options) noexcept;
    bool begin_mission_transition(std::int64_t id);
    void close_map();
    godot::Dictionary get_map_status() const;
    godot::Dictionary get_map_world_status() const;
    godot::Array get_map_objects() const;
    godot::Dictionary get_map_object(std::int64_t world, std::int64_t id) const;
    godot::Dictionary get_map_resource(int x, int y) const;
    bool set_map_object_health(std::int64_t world, std::int64_t id, int health);
    bool set_map_unit_cheat_health(std::int64_t world, std::int64_t id, int health);
    bool set_map_object_level(std::int64_t world, std::int64_t id, int level);
    bool grant_map_player_power();
    godot::Array get_debug_commands() const;
    std::int64_t get_debug_command_world() const;
    godot::Dictionary get_debug_performance(std::int64_t expected_world) const;
    godot::Dictionary execute_debug_command(const godot::String& text, std::int64_t expected_world);
    void record_host_performance(std::uint64_t expected_world, const game::DebugHostPerformance&) noexcept;
    bool set_map_building_enabled(std::int64_t world, std::int64_t id, bool enabled);
    godot::Dictionary harvest_map_resource(int x, int y, int requested);
    bool submit_input(const game::GameInputEvent&, game::GameInputResult&) noexcept;
    bool read_cursor(game::GameCursorFrame&, std::uint8_t* rgba = nullptr, std::uint32_t capacity = 0) noexcept;
    bool update_view(double seconds) noexcept;
    game::DrawingStatus advance_map(double seconds, game::PresentationFrameHandle&,
                                    game::MapDrawStatistics&, bool& rendered) noexcept;


private:
    void begin_map_request(const godot::String& filename, const game::SkirmishOptions* options, int campaign_difficulty = -1);
    enum class LoadState { idle, loading, complete, failed, cancelled };
    struct Progress {
        LoadState state = LoadState::idle;
        unsigned completed = 0, total = 108;
        std::string current, error;
        std::vector<std::string> mounted;
        size_t index_entries = 0;
    };
    void load_resources(std::stop_token stop, const std::string& directory) noexcept;
    void publish_failure(const char* message) noexcept;
    void close_resources() noexcept;
    std::atomic<bool> terminal_failure_{false};
    void publish(const Progress& progress);
    mutable std::mutex progress_mutex_;
    Progress progress_;
    enum class MapState { empty, loading, ready, failed, cancelled };
    MapState map_state_ = MapState::empty;
    std::string map_error_;
    std::uint64_t map_revision_ = 0;
    game::MapViewHandle* map_ = nullptr;
    game::AudioSession* audio_ = nullptr;
    game::MovieSession* movies_ = nullptr;
    bool audio_preferences_set_ = false;
    int speed_level_ = 5;
    bool host_focused_ = true, host_paused_ = false;
    float effects_volume_ = 0.8f, speech_volume_ = 0.8f, music_volume_ = 0.8f;
    std::unique_ptr<RA2AudioOutput> audio_output_;
    // Platform service lifetime only. All package slots/arrays/list live in YRpp.
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> files_{nullptr, game::destroy_resources};
    std::jthread worker_;
};
