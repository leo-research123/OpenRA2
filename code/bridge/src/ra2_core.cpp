#include "api/drawing_diagnostics.hpp"
#include "bridge/ra2_core.hpp"
#include "core_call_scope.hpp"
#include "map_input.hpp"
#include "bridge/audio_output.hpp"
#include <godot_cpp/variant/rect2i.hpp>
#include "api/version.hpp"
#include "api/projectile_diagnostics.hpp"
#include "api/runtime_diagnostics.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "api/resource_queries.hpp"
#include <exception>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

RA2Core::RA2Core() = default;
void RA2Core::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_audio_status"), &RA2Core::get_audio_status);
    godot::ClassDB::bind_method(godot::D_METHOD("set_audio_volumes", "effects", "speech", "music"),
                                &RA2Core::set_audio_volumes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_audio_volumes"), &RA2Core::get_audio_volumes);
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_speed_level", "speed_level"), &RA2Core::set_game_speed_level);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hotkeys"), &RA2Core::get_hotkeys);
    godot::ClassDB::bind_method(godot::D_METHOD("set_hotkeys", "bindings"), &RA2Core::set_hotkeys);
    godot::ClassDB::bind_method(godot::D_METHOD("encode_hotkey", "event"), &RA2Core::encode_hotkey);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_world_status"), &RA2Core::get_map_world_status);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_objects"), &RA2Core::get_map_objects);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_object", "world", "id"), &RA2Core::get_map_object);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_resource", "x", "y"), &RA2Core::get_map_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_object_health", "world", "id", "health"),
                                &RA2Core::set_map_object_health);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_unit_cheat_health", "world", "id", "health"),
                                &RA2Core::set_map_unit_cheat_health);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_object_level", "world", "id", "level"),
                                &RA2Core::set_map_object_level);
    godot::ClassDB::bind_method(godot::D_METHOD("grant_map_player_power"), &RA2Core::grant_map_player_power);
    godot::ClassDB::bind_method(godot::D_METHOD("get_debug_commands"), &RA2Core::get_debug_commands);
    godot::ClassDB::bind_method(godot::D_METHOD("get_debug_command_world"), &RA2Core::get_debug_command_world);
    godot::ClassDB::bind_method(godot::D_METHOD("get_debug_performance", "expected_world"),
                                &RA2Core::get_debug_performance);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_debug_command", "text", "expected_world"),
                                &RA2Core::execute_debug_command);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_building_enabled", "world", "id", "enabled"),
                                &RA2Core::set_map_building_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("harvest_map_resource", "x", "y", "requested"),
                                &RA2Core::harvest_map_resource);

    godot::ClassDB::bind_method(godot::D_METHOD("begin_map_loading", "filename"), &RA2Core::begin_map_loading);
    godot::ClassDB::bind_method(godot::D_METHOD("begin_campaign_loading", "filename", "difficulty"), &RA2Core::begin_campaign_loading);
    godot::ClassDB::bind_method(godot::D_METHOD("begin_skirmish_loading", "filename", "options"), &RA2Core::begin_skirmish_loading);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_status"), &RA2Core::get_map_status);
    godot::ClassDB::bind_method(godot::D_METHOD("close_map"), &RA2Core::close_map);
    godot::ClassDB::bind_method(godot::D_METHOD("load_ini", "filename"), &RA2Core::load_ini);
    godot::ClassDB::bind_method(godot::D_METHOD("try_get_localized_string", "label", "output"),
                                &RA2Core::try_get_localized_string);
    godot::ClassDB::bind_method(godot::D_METHOD("prepare_map_resources"), &RA2Core::prepare_map_resources);
    godot::ClassDB::bind_method(godot::D_METHOD("get_version"), &RA2Core::get_version);
    godot::ClassDB::bind_method(godot::D_METHOD("begin_resource_loading", "directory"),
                                &RA2Core::begin_resource_loading);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resource_progress"), &RA2Core::get_resource_progress);
    godot::ClassDB::bind_method(godot::D_METHOD("cancel_resource_loading"), &RA2Core::cancel_resource_loading);
}

void RA2Core::begin_resource_loading(const godot::String& directory) {
    const auto utf8 = directory.utf8();
    cancel_resource_loading();
    close_resources();
    try {
        const auto logDirectory = godot::ProjectSettings::get_singleton()->globalize_path("user://logs");
        const auto runtimePath = logDirectory.path_join("runtime.log");
        const auto runtimeUtf8 = runtimePath.utf8();
        if (game::configure_runtime_log(runtimeUtf8.get_data(), utf8.get_data()))
            godot::UtilityFunctions::print("Runtime diagnostics: ", runtimePath);
        else
            godot::UtilityFunctions::printerr("Could not open runtime diagnostics: ", runtimePath);
        if (!game::configure_runtime_crash_capture())
            godot::UtilityFunctions::printerr("Could not enable native crash capture");
        const auto logPath = logDirectory.path_join("projectiles.log");
        const auto logUtf8 = logPath.utf8();
        if (game::configure_projectile_log(logUtf8.get_data()))
            godot::UtilityFunctions::print("Projectile diagnostics: ", logPath);
        else {
            game::configure_projectile_log(nullptr);
            godot::UtilityFunctions::printerr("Could not open projectile diagnostics: ", logPath);
        }
    } catch (...) {
        game::configure_projectile_log(nullptr);
    }
    terminal_failure_.store(false, std::memory_order_release);
    try {
        Progress initial;
        initial.state = LoadState::loading;
        publish(initial);
        worker_ = std::jthread(
            [this, path = std::string(utf8.get_data())](std::stop_token stop) noexcept { load_resources(stop, path); });
    } catch (const std::exception& e) {
        publish_failure(e.what());
    } catch (...) {
        publish_failure("Unknown resource loading failure");
    }
}

godot::Dictionary RA2Core::get_resource_progress() const {
    Progress p;
    {
        const std::lock_guard lock(progress_mutex_);
        p = progress_;
    }
    if (terminal_failure_.load(std::memory_order_acquire)) {
        p.state = LoadState::failed;
        if (p.error.empty()) p.error = "Resource loading failed while reporting progress";
    }
    constexpr const char* states[] = {"idle", "loading", "complete", "failed", "cancelled"};
    godot::Dictionary result;
    result["state"] = states[static_cast<unsigned>(p.state)];
    result["completed"] = int64_t(p.completed);
    result["total"] = int64_t(p.total);
    result["current"] = godot::String::utf8(p.current.c_str());
    result["error"] = godot::String::utf8(p.error.c_str());
    result["index_entries"] = int64_t(p.index_entries);
    godot::PackedStringArray mounts;
    for (const auto& name : p.mounted)
        mounts.append(godot::String::utf8(name.c_str()));
    result["mounted"] = mounts;
    return result;
}

RA2Core::~RA2Core() {
    cancel_resource_loading();
    close_resources();
    game::configure_runtime_log(nullptr, nullptr);
}
void RA2Core::cancel_resource_loading() {
    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join();
    }
}
void RA2Core::publish(const Progress& progress) {
    const std::lock_guard lock(progress_mutex_);
    progress_ = progress;
}
void RA2Core::close_resources() noexcept {
    // Owner-thread caller has already joined the worker. Do not unload another
    // host's process-global SHP state when this RA2Core owns no file environment.
    if (audio_) game::stop_audio_session(*audio_);
    game::destroy_map_view(map_);
    // Owner-thread teardown follows a joined worker, like resource publication.
    map_state_ = MapState::empty;
    map_error_.clear();
    ++map_revision_;
    audio_output_.reset();
    game::destroy_movie_session(movies_);
    game::destroy_audio_session(audio_);
    if (!files_) return;
    game::unload_shapes();
    files_.reset();
}
void RA2Core::publish_failure(const char* message) noexcept {
    // Even allocation or mutex failure while reporting cannot escape a worker.
    terminal_failure_.store(true, std::memory_order_release);
    try {
        const std::lock_guard lock(progress_mutex_);
        progress_.state = LoadState::failed;
        try {
            progress_.error = message;
        } catch (...) {
            progress_.error.clear();
        }
    } catch (...) {
    }
}
void RA2Core::load_resources(std::stop_token stop, const std::string& directory) noexcept {
    Progress p;
    p.state = LoadState::loading;
    auto report = [&] {
        p.mounted.clear();
        p.index_entries = 0;
        if (files_) {
            std::uint32_t packages=0;
            game::resource_package_count(*files_, packages);
            for (std::uint32_t i=0; i<packages; ++i) {
                const char* name=nullptr; std::uint32_t entries=0;
                if (game::resource_package(*files_, i, name, entries)) {
                    p.mounted.emplace_back(name ? name : ""); p.index_entries += entries;
                }
            }
        }
        publish(p);
    };
    auto cancelled = [&] {
        if (!stop.stop_requested()) return false;
        p.state = LoadState::cancelled;
        report();
        return true;
    };
    try {
        if (cancelled()) return;
        game::ResourceHandle* resources = nullptr;
        if (!game::create_resources(directory, resources, p.error)) {
            p.state = LoadState::failed;
            report();
            return;
        }
        files_.reset(resources);

        struct Observation {
            Progress& progress;
            decltype(report)& publish;
            std::stop_token stop;
            std::exception_ptr failure;
            void update(const char* name, unsigned count, unsigned total) noexcept {
                // UI allocation errors must not interrupt the original entry.
                if (failure) return;
                try {
                    progress.current = name;
                    progress.completed = count;
                    progress.total = total;
                    publish();
                } catch (...) {
                    failure = std::current_exception();
                }
            }
        } observation{p, report, stop, {}};
        const auto observe = [](void* data, const char* name, unsigned count, unsigned total) noexcept {
            static_cast<Observation*>(data)->update(name, count, total);
        };
        const auto stopped = [](void* data) noexcept { return static_cast<Observation*>(data)->stop.stop_requested(); };
        const auto result = game::load_resources(*files_, {&observation, observe, stopped}, p.error);
        if (observation.failure) std::rethrow_exception(observation.failure);
        if (result == game::ResourceLoadResult::failed) {
            p.state = LoadState::failed;
        } else {
            p.state = result == game::ResourceLoadResult::cancelled ? LoadState::cancelled : LoadState::complete;
        }
        report();
    } catch (const std::exception& e) {
        publish_failure(e.what());
    } catch (...) {
        publish_failure("Unknown resource loading failure");
    }
}

godot::String RA2Core::get_version() const { return godot::String::utf8(game::project_version); }

bool RA2Core::try_get_localized_string(const godot::String& label, godot::Dictionary output) noexcept {
    try {
        if (terminal_failure_.load(std::memory_order_acquire)) return false;
        {
            const std::lock_guard lock(progress_mutex_);
            if (progress_.state != LoadState::complete || map_state_ == MapState::loading) return false;
        }
        if (worker_.joinable()) worker_.join();
        if (!files_) return false;
        const auto name = label.utf8();
        std::uint32_t length=0;
        if (!game::localized_string(*files_, name.get_data(), nullptr, 0, length)) return false;
        std::vector<std::uint16_t> units(length+1);
        if (!game::localized_string(*files_, name.get_data(), units.data(), units.size(), length)) return false;
        output["text"] = godot::String::utf16(reinterpret_cast<const char16_t*>(units.data()), length);
        return true;
    } catch (...) {
        return false;
    }
}

bool RA2Core::prepare_map_resources() noexcept {
    try {
        if (terminal_failure_.load(std::memory_order_acquire)) return false;
        {
            const std::lock_guard lock(progress_mutex_);
            if (progress_.state != LoadState::complete || map_state_ != MapState::empty) return false;
        }
        if (worker_.joinable()) worker_.join();
        if (!files_) return false;
        return game::mount_map_resources(*files_);
    } catch (...) {
        return false;
    }
}

// Owner-thread API, like begin/cancel. Files are only consumed after the worker
// publishes completion; each parsed INI owns its metadata independently.
godot::Ref<RA2INI> RA2Core::load_ini(const godot::String& filename) {
    if (terminal_failure_.load(std::memory_order_acquire)) return {};
    {
        const std::lock_guard lock(progress_mutex_);
        if (progress_.state != LoadState::complete || map_state_ == MapState::loading) return {};
    }
    if (worker_.joinable()) worker_.join();
    if (!files_) return {};
    try {
        godot::Ref<RA2INI> result;
        result.instantiate();
        const auto name = filename.utf8();
        if (!game::load_ini(*files_, name.get_data(), result->ini_)) return {};
        result->initialize_text_encoding();
        return result;
    } catch (const std::exception& e) {
        ERR_PRINT(godot::String::utf8(e.what()));
        return {};
    }
}

void RA2Core::close_map() {
    game::CoreCallScope floating_point;
    cancel_resource_loading();  // Joins either resource or map work before teardown.
    const bool had_map = map_ != nullptr;
    if (audio_) game::stop_audio_session(*audio_);
    // Stop the host playback while the scene still runs. AudioServer retires
    // stopped playbacks on a later mix/update, not in AudioStreamPlayer::stop.
    audio_output_.reset();
    game::destroy_map_view(map_);
    game::destroy_movie_session(movies_);
    // Drawing now uses SHPReference's original persistent storage. The host
    // releases it after all core consumers stop; queued GPU frames own copies.
    // Keep reference identities/name-cache entries, but not old theater bytes.
    if (had_map) game::unload_shapes();
    const std::lock_guard lock(progress_mutex_);
    map_state_ = MapState::empty;
    map_error_.clear();
    ++map_revision_;
}
void RA2Core::begin_map_loading(const godot::String& filename) {
    begin_map_request(filename, nullptr);
}
void RA2Core::begin_campaign_loading(const godot::String& filename, int difficulty) {
    if (difficulty < 0 || difficulty > 2) {
        close_map();
        const std::lock_guard lock(progress_mutex_);
        map_state_ = MapState::failed;
        map_error_ = "Invalid campaign difficulty";
        return;
    }
    begin_map_request(filename, nullptr, difficulty);
}
void RA2Core::begin_skirmish_loading(const godot::String& filename, const godot::Dictionary& options) noexcept {
    try {
        game::SkirmishOptions setup;
        const auto integer = [&](const char* key, std::int64_t low, std::int64_t high) {
            const godot::Variant value = options.get(key, godot::Variant());
            if (value.get_type() != godot::Variant::INT || std::int64_t(value) < low || std::int64_t(value) > high)
                throw std::runtime_error("Invalid skirmish integer option");
            return std::int64_t(value);
        };
        const auto boolean = [&](const char* key) {
            const godot::Variant value = options.get(key, godot::Variant());
            if (value.get_type() != godot::Variant::BOOL) throw std::runtime_error("Invalid skirmish boolean option");
            return bool(value);
        };
        setup.money = int(integer("money", 0, 1000000));
        setup.unit_count = int(integer("units", 0, 100));
        setup.game_speed = int(integer("game_speed", 0, 6));
        setup.seed = std::uint32_t(integer("seed", 0, 0xFFFFFFFF));
        setup.short_game = boolean("ShortGame");
        setup.super_weapons = boolean("SuperWeaponsAllowed");
        setup.build_off_ally = boolean("BuildOffAlly");
        setup.mcv_redeploy = boolean("MCVRedeploys");
        setup.crates = boolean("Crates");
        setup.bridge_destruction = boolean("BridgeDestruction");
        const godot::Variant slots = options.get("players", godot::Variant());
        if (slots.get_type() != godot::Variant::PACKED_INT32_ARRAY) throw std::runtime_error("Invalid skirmish player list");
        const godot::PackedInt32Array values = slots;
        if (values.size() != game::SkirmishSlotCount * 5) throw std::runtime_error("Skirmish player slot count mismatch");
        for (int i = 0; i < game::SkirmishSlotCount; ++i)
            setup.players[i] = {values[i * 5], values[i * 5 + 1], values[i * 5 + 2], values[i * 5 + 3], values[i * 5 + 4]};
        begin_map_request(filename, &setup);
    } catch (...) {
        close_map();
        const std::lock_guard lock(progress_mutex_);
        map_state_ = MapState::failed;
        map_error_ = "Invalid skirmish request";
    }
}
void RA2Core::begin_map_request(const godot::String& filename, const game::SkirmishOptions* options, int campaign_difficulty) {
    game::CoreCallScope floating_point;
    close_map();
    {
        const std::lock_guard lock(progress_mutex_);
        if (progress_.state != LoadState::complete || !files_) {
            map_state_ = MapState::failed;
            map_error_ = "Resources are not ready";
            return;
        }
        map_state_ = MapState::loading;
    }
    try {
        if (!audio_) {
            game::create_audio_session(RA2AudioOutput::sample_rate, audio_);
        }
        if (!movies_) {
            if (!game::create_movie_session(audio_ ? RA2AudioOutput::sample_rate : 0, movies_))
                throw std::runtime_error("Cannot initialize movie backend");
        }
        if (audio_ && !audio_output_) {
            audio_output_ = std::make_unique<RA2AudioOutput>();
            if (!audio_output_->open(*audio_, movies_)) {
                audio_output_.reset();
                game::destroy_audio_session(audio_);
                game::destroy_movie_session(movies_);
                if (!game::create_movie_session(0, movies_))
                    throw std::runtime_error("Cannot initialize video-only movie backend");
            }
        }
        const auto utf8 = filename.utf8();
        const bool skirmish = options != nullptr;
        const game::SkirmishOptions setup = options ? *options : game::SkirmishOptions{};
        if (skirmish) {
            const std::lock_guard lock(progress_mutex_);
            speed_level_ = setup.game_speed ? 7 - setup.game_speed : 9;
        }
        worker_ = std::jthread([this, name = std::string(utf8.get_data()), skirmish, setup, campaign_difficulty](std::stop_token stop) noexcept {
            bool ok = false;
            try {
                int initial_speed;
                {
                    const std::lock_guard lock(progress_mutex_);
                    initial_speed = speed_level_;
                }
                if (audio_ && !game::load_audio_session(*audio_, *files_))
                    godot::UtilityFunctions::printerr("Audio unavailable: ", game::audio_session_error(*audio_));
                ok = game::create_map_view(*files_, map_) && game::set_game_speed_level(*map_, initial_speed) &&
                     (campaign_difficulty < 0 || game::set_campaign_difficulty(*map_, campaign_difficulty)) &&
                     (skirmish ? game::load_skirmish_view(*map_, name.data(), static_cast<std::uint32_t>(name.size()), setup)
                               : game::load_map_view(*map_, name.data(), static_cast<std::uint32_t>(name.size())));
                std::string error = ok ? "" : map_ ? game::map_view_error(*map_) : "Another map owns the world";
                if (!ok || stop.stop_requested()) game::destroy_map_view(map_);
                const std::lock_guard lock(progress_mutex_);
                if (ok && !stop.stop_requested()) {
                    ok = game::set_game_speed_level(*map_, speed_level_) &&
                         game::set_game_view_activity(*map_, host_focused_, host_paused_);
                    if (!ok) {
                        game::destroy_map_view(map_);
                        error = "Could not apply game settings";
                    }
                }
                if (audio_ && audio_preferences_set_)
                    game::set_audio_levels(*audio_, effects_volume_, speech_volume_, music_volume_);
                map_error_ = std::move(error);
                map_state_ = stop.stop_requested() ? MapState::cancelled : ok ? MapState::ready : MapState::failed;
            } catch (...) {
                game::destroy_map_view(map_);
                try {
                    const std::lock_guard lock(progress_mutex_);
                    map_state_ = MapState::failed;
                    map_error_ = "Map loading failed";
                } catch (...) {
                }
            }
        });
    } catch (...) {
        const std::lock_guard lock(progress_mutex_);
        map_state_ = MapState::failed;
        map_error_ = "Cannot start map loader";
    }
}
bool RA2Core::begin_mission_transition(std::int64_t id) {
    game::CoreCallScope floating_point;
    {
        const std::lock_guard lock(progress_mutex_);
        if (map_state_ == MapState::loading || !map_ || id <= 0) return false;
    }
    cancel_resource_loading();
    if (!game::begin_mission_transition(*map_, static_cast<std::uint64_t>(id))) return false;
    {
        const std::lock_guard lock(progress_mutex_);
        map_state_ = MapState::loading;
        map_error_.clear();
        ++map_revision_;
    }
    try {
        worker_ = std::jthread([this](std::stop_token stop) noexcept {
            game::CoreCallScope floating_point;
            const bool ok = game::load_mission_transition(*map_);
            const std::lock_guard lock(progress_mutex_);
            // The handle and prepared Evade state survive failure for an explicit retry.
            if (!ok) {
                try { map_error_ = game::map_view_error(*map_); } catch (...) {}
            }
            if (ok) game::set_game_view_activity(*map_, host_focused_, host_paused_);
            map_state_ = stop.stop_requested() ? MapState::cancelled : ok ? MapState::ready : MapState::failed;
        });
        return true;
    } catch (...) {
        game::fail_mission_transition(*map_, "Cannot start mission loader");
        const std::lock_guard lock(progress_mutex_);
        map_state_ = MapState::failed;
        try { map_error_ = game::map_view_error(*map_); } catch (...) {}
        return true;
    }
}
godot::Dictionary RA2Core::get_map_status() const {
    game::CoreCallScope floating_point;
    const std::lock_guard lock(progress_mutex_);
    constexpr const char* states[]{"empty", "loading", "ready", "failed", "cancelled"};
    godot::Dictionary result;
    result["state"] = states[static_cast<unsigned>(map_state_)];
    result["error"] = godot::String::utf8(map_error_.c_str());
    result["revision"] = static_cast<std::int64_t>(map_revision_);
    result["speed_level"] = speed_level_;
    if (map_ && map_state_ != MapState::loading) {
        game::MissionTransitionInfo transition;
        if (game::get_mission_transition(*map_, transition)) {
            result["mission_id"] = static_cast<std::int64_t>(transition.id);
            result["mission_transition"] = static_cast<int>(transition.state);
            result["mission_target"] = godot::String::utf8(transition.filename);
            result["mission_error"] = godot::String::utf8(transition.error);
        }
    }
    game::MapViewInfo info{};
    if (map_state_ == MapState::ready && map_ && game::get_map_view_info(*map_, info)) {
        result["error"] = godot::String::utf8(game::map_view_error(*map_));
        result["width"] = info.width;
        result["height"] = info.height;
        result["theater"] = info.theater;
        result["camera_x"] = info.camera_x;
        result["camera_y"] = info.camera_y;
        result["camera_revision"] = static_cast<std::int64_t>(info.camera_generation);
        game::MapWorldSnapshot world{};
        if (game::get_map_world_snapshot(*map_, world)) {
            result["presentation_revision"] = static_cast<std::int64_t>(world.presentation_revision);
            result["resource_revision"] = static_cast<std::int64_t>(world.resource_revision);
            result["simulation_tick"] = static_cast<std::int64_t>(world.simulation_tick);
        }
        game::GameViewState ui{};
        game::DebugModes modes;
        if (game::get_debug_modes(*map_, modes)) {
            result["teleport_mode"] = modes.teleport;
            result["fog_visible"] = modes.fog_visible;
            result["selection_mode"] = modes.selection;
            result["control_mode"] = modes.control;
        }
        if (game::get_game_view_state(*map_, ui)) {
            result["ui_revision"] = static_cast<std::int64_t>(ui.revision);
            result["paused"] = ui.paused;
            result["pointer_captured"] = ui.pointer_captured;
            result["active_tab"] = ui.active_tab;
            result["command_bar_expanded"] = ui.command_bar_expanded;
            result["special_dialog"] = ui.special_dialog;
            result["mission_result"] = static_cast<int>(ui.result);
        }
        game::GameViewLayout layout{};
        if (game::get_game_view_layout(*map_, layout)) {
            const auto rect = [](const RectangleStruct& r) { return godot::Rect2i(r.X, r.Y, r.Width, r.Height); };
            result["canvas_bounds"] = rect(layout.canvas);
            result["map_bounds"] = rect(layout.map);
            result["sidebar_bounds"] = rect(layout.sidebar);
            result["radar_bounds"] = rect(layout.radar);
            result["hud_bounds"] = rect(layout.command_bar);
        }
        result["current_frame"] = ui.current_frame;
        result["game_speed"] = ui.game_speed;
        result["speed_level"] = ui.speed_level;
        result["target_logic_fps"] = ui.target_logic_fps;
        result["batch_steps"] = ui.batch_steps;
        result["batch_limited"] = ui.batch_limited;
        result["logic_iterations"] = static_cast<std::int64_t>(ui.logic_iterations);
        result["elapsed_wall_seconds"] = ui.elapsed_wall_seconds;
        game::MapRadarInfo radar{};
        const bool available = game::get_map_radar_info(*map_, radar);
        result["radar_available"] = available && ui.radar_available;
        result["radar_terrain_loaded"] = available;
        if (available) {
            result["radar_x"] = radar.x;
            result["radar_y"] = radar.y;
            result["radar_width"] = radar.width;
            result["radar_height"] = radar.height;
            result["radar_frame_valid"] = radar.viewport_valid;
            result["radar_frame_x"] = radar.viewport_x;
            result["radar_frame_y"] = radar.viewport_y;
            result["radar_frame_width"] = radar.viewport_width;
            result["radar_frame_height"] = radar.viewport_height;
        }
    }
    return result;
}
bool RA2Core::submit_input(const game::GameInputEvent& event, game::GameInputResult& result) noexcept {
    game::CoreCallScope floating_point;
    try {
        const std::lock_guard lock(progress_mutex_);
        return map_state_ == MapState::ready && map_ && game::submit_game_input(*map_, event, result);
    } catch (...) {
        return false;
    }
}
bool RA2Core::read_cursor(game::GameCursorFrame& frame, std::uint8_t* rgba, std::uint32_t capacity) noexcept {
    game::CoreCallScope floating_point;
    try {
        const std::lock_guard lock(progress_mutex_);
        return map_state_ == MapState::ready && map_ && game::read_game_cursor(*map_, frame, rgba, capacity);
    } catch (...) {
        return false;
    }
}
bool RA2Core::update_view(double seconds) noexcept {
    game::CoreCallScope floating_point;
    try {
        const std::lock_guard lock(progress_mutex_);
        return map_state_ == MapState::ready && map_ && game::update_game_view(*map_, seconds);
    } catch (...) {
        return false;
    }
}
game::DrawingStatus RA2Core::advance_map(double seconds, game::PresentationFrameHandle& frame,
                                         game::MapDrawStatistics& stats, bool& rendered) noexcept {
    game::CoreCallScope floating_point;
    rendered = false;
    game::begin_drawing_diagnostics();
    try {
        const std::lock_guard lock(progress_mutex_);
        if (map_state_ != MapState::ready || !map_) return game::DrawingStatus::unavailable;
        return game::advance_presentation_frame(*map_, frame, seconds, stats, rendered);
    } catch (...) {
        return game::DrawingStatus::backend_failure;
    }
}


godot::Dictionary RA2Core::get_audio_status() const {
    const std::lock_guard lock(progress_mutex_);
    godot::Dictionary result;
    result["output_playback_id"] = audio_output_ ? audio_output_->playback_id() : std::uint64_t(0);
    result["stream_sample_rate"] = audio_output_ ? RA2AudioOutput::sample_rate : 0u;
    if (map_state_ == MapState::loading) {
        result["state"] = "loading";
        return result;
    }
    if (movies_) {
        game::MovieStatistics movie;
        if (game::get_movie_statistics(*movies_, movie)) {
            result["movie_created"] = movie.created;
            result["movie_destroyed"] = movie.destroyed;
            result["movie_video_frames"] = movie.video_frames;
            result["movie_audio_frames"] = movie.audio_frames;
            result["movie_mixed_frames"] = movie.mixed_frames;
            result["movie_active"] = movie.active;
            result["movie_failures"] = movie.failures;
            result["movie_error"] = game::movie_session_error(*movies_);
        }
    }
    game::AudioStatistics stats;
    const bool present = audio_ && game::get_audio_statistics(*audio_, stats);
    result["state"] = present && stats.resources_ready ? "ready" : "unavailable";
    if (present) {
        result["events"] = stats.active_events;
        result["streams"] = stats.active_streams;
        result["definitions"] = stats.sound_definitions;
        result["missing_samples"] = stats.missing_samples;
        result["decode_failures"] = stats.decode_failures;
        result["underruns"] = stats.underruns;
        result["command_overflows"] = stats.command_overflows;
        result["error"] = game::audio_session_error(*audio_);
    }
    return result;
}
bool RA2Core::set_audio_volumes(double effects, double speech, double music) noexcept {
    if (!std::isfinite(effects) || !std::isfinite(speech) || !std::isfinite(music) || effects < 0 || effects > 1 ||
        speech < 0 || speech > 1 || music < 0 || music > 1)
        return false;
    try {
        const std::lock_guard lock(progress_mutex_);
        // Menu preferences may precede device creation or arrive during loading.
        // The loader applies the latest values before publishing the ready state.
        effects_volume_ = float(effects);
        speech_volume_ = float(speech);
        music_volume_ = float(music);
        audio_preferences_set_ = true;
        return map_state_ == MapState::loading || !audio_ ||
               game::set_audio_levels(*audio_, effects_volume_, speech_volume_, music_volume_);
    } catch (...) {
        return false;
    }
}
godot::Dictionary RA2Core::get_audio_volumes() const noexcept {
    try {
        const std::lock_guard lock(progress_mutex_);
        float effects = effects_volume_, speech = speech_volume_, music = music_volume_;
        if (audio_ && map_state_ != MapState::loading) game::get_audio_levels(*audio_, effects, speech, music);
        godot::Dictionary result;
        result["effects"] = effects;
        result["speech"] = speech;
        result["music"] = music;
        return result;
    } catch (...) {
        return {};
    }
}
bool RA2Core::set_game_speed_level(std::int64_t level) noexcept {
    if (level < 1 || level > 9) return false;
    try {
        const std::lock_guard lock(progress_mutex_);
        // The loading worker owns original globals until ready publication.
        // Queue preference edits there, just as for volume changes.
        if (map_state_ == MapState::ready && map_ && !game::set_game_speed_level(*map_, int(level))) return false;
        speed_level_ = int(level);
        return true;
    } catch (...) {
        return false;
    }
}
void RA2Core::set_activity(bool focused, bool paused) noexcept {
    try {
        const std::lock_guard lock(progress_mutex_);
        host_focused_ = focused;
        host_paused_ = paused;
        if (map_state_ == MapState::ready && map_) {
            game::set_game_view_activity(*map_, focused, paused);
        }
    } catch (...) {
    }
}
godot::Array RA2Core::get_hotkeys() noexcept {
    game::CoreCallScope floating_point;
    try {
        const std::lock_guard lock(progress_mutex_);
        if (!files_ || progress_.state != LoadState::complete || map_state_ == MapState::loading) return {};
        struct Read {
            godot::Array items;
            bool success = true;
        } read;
        std::string error;
        if (!game::with_resources(
                *files_,
                [](void* p) {
                    auto& r = *static_cast<Read*>(p);
                    for (unsigned i = 0; i < game::player_hotkey_count(); ++i) {
                        game::PlayerHotkeyBinding binding;
                        if (!game::read_player_hotkey(i, binding)) {
                            r.success = false;
                            return;
                        }
                        godot::Dictionary item;
                        item["command"] = godot::String::utf8(binding.command);
                        std::u16string units;
                        for (auto* text = binding.name; text && *text; ++text)
                            units.push_back(static_cast<char16_t>(*text));
                        item["name"] = godot::String::utf16(units.c_str());
                        item["key"] = binding.key;
                        item["default_key"] = binding.default_key;
                        r.items.append(item);
                    }
                },
                &read, error) ||
            !read.success)
            return {};
        return read.items;
    } catch (...) {
        return {};
    }
}
bool RA2Core::set_hotkeys(const godot::Dictionary& bindings) noexcept {
    game::CoreCallScope floating_point;
    try {
        const auto items = get_hotkeys();
        if (items.is_empty()) return false;
        std::vector<unsigned> keys(items.size());
        for (std::int64_t i = 0; i < items.size(); ++i) {
            const godot::Dictionary item = items[i];
            const auto key = std::int64_t(bindings.get(item["command"], item["key"]));
            if (key < 0 || key > 0x17FF) return false;
            keys[i] = unsigned(key);
        }
        const std::lock_guard lock(progress_mutex_);
        if (map_state_ == MapState::loading) return false;
        return game::set_player_hotkeys(keys.data(), static_cast<unsigned>(keys.size()));
    } catch (...) {
        return false;
    }
}
std::int64_t RA2Core::encode_hotkey(const godot::Ref<godot::InputEvent>& event) noexcept {
    game::GameInputEvent input;
    bool latch = false;
    if (!game::encode_canvas_input(event, {1, 1}, {1, 1}, input, latch) || input.kind != game::GameInputKind::key ||
        !input.pressed || input.code == 16 || input.code == 17 || input.code == 18)
        return 0;
    return input.code | (input.modifiers << 8);
}
