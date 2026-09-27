#include "bridge/ra2_core.hpp"
#include <godot_cpp/variant/rect2i.hpp>
#include "api/version.hpp"
#include "api/projectile_diagnostics.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/MixFileClass.h"
#include <exception>
#include <stdexcept>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

void RA2Core::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_world_status"), &RA2Core::get_map_world_status);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_objects"), &RA2Core::get_map_objects);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_object", "world", "id"), &RA2Core::get_map_object);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_resource", "x", "y"), &RA2Core::get_map_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_object_health", "world", "id", "health"), &RA2Core::set_map_object_health);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_unit_cheat_health", "world", "id", "health"), &RA2Core::set_map_unit_cheat_health);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_object_level", "world", "id", "level"), &RA2Core::set_map_object_level);
    godot::ClassDB::bind_method(godot::D_METHOD("grant_map_player_power"), &RA2Core::grant_map_player_power);
    godot::ClassDB::bind_method(godot::D_METHOD("set_map_building_enabled", "world", "id", "enabled"), &RA2Core::set_map_building_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("harvest_map_resource", "x", "y", "requested"), &RA2Core::harvest_map_resource);

    godot::ClassDB::bind_method(godot::D_METHOD("begin_map_loading", "filename"), &RA2Core::begin_map_loading);
    godot::ClassDB::bind_method(godot::D_METHOD("get_map_status"), &RA2Core::get_map_status);
    godot::ClassDB::bind_method(godot::D_METHOD("close_map"), &RA2Core::close_map);
    godot::ClassDB::bind_method(godot::D_METHOD("load_ini", "filename"), &RA2Core::load_ini);
    godot::ClassDB::bind_method(godot::D_METHOD("prepare_map_resources"), &RA2Core::prepare_map_resources);
    godot::ClassDB::bind_method(godot::D_METHOD("get_version"), &RA2Core::get_version);
    godot::ClassDB::bind_method(godot::D_METHOD("begin_resource_loading", "directory"), &RA2Core::begin_resource_loading);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resource_progress"), &RA2Core::get_resource_progress);
    godot::ClassDB::bind_method(godot::D_METHOD("cancel_resource_loading"), &RA2Core::cancel_resource_loading);
}

void RA2Core::begin_resource_loading(const godot::String& directory) {
    const auto utf8 = directory.utf8();
    cancel_resource_loading();
    close_resources();
    try {
        const auto logDirectory=godot::ProjectSettings::get_singleton()->globalize_path("user://logs");
        const auto logPath=logDirectory.path_join("projectiles.log");const auto logUtf8=logPath.utf8();
        if(game::configure_projectile_log(logUtf8.get_data()))godot::UtilityFunctions::print("Projectile diagnostics: ",logPath);
        else {game::configure_projectile_log(nullptr);godot::UtilityFunctions::printerr("Could not open projectile diagnostics: ",logPath);}
    }catch(...){game::configure_projectile_log(nullptr);}
    terminal_failure_.store(false, std::memory_order_release);
    try {
        Progress initial;
        initial.state = LoadState::loading;
        publish(initial);
        worker_ = std::jthread([this, path = std::string(utf8.get_data())](std::stop_token stop) noexcept {
            load_resources(stop, path);
        });
    } catch (const std::exception& e) {
        publish_failure(e.what());
    } catch (...) {
        publish_failure("Unknown resource loading failure");
    }
}

godot::Dictionary RA2Core::get_resource_progress() const {
    Progress p;
    { const std::lock_guard lock(progress_mutex_); p = progress_; }
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
    for (const auto& name : p.mounted) mounts.append(godot::String::utf8(name.c_str()));
    result["mounted"] = mounts;
    return result;
}

RA2Core::~RA2Core() {
    cancel_resource_loading();
    close_resources();
}
void RA2Core::cancel_resource_loading() {
    if (worker_.joinable()) { worker_.request_stop(); worker_.join(); }
}
void RA2Core::publish(const Progress& progress) {
    const std::lock_guard lock(progress_mutex_);
    progress_ = progress;
}
void RA2Core::close_resources() noexcept {
    // Owner-thread caller has already joined the worker. Do not unload another
    // host's process-global SHP state when this RA2Core owns no file environment.
    game::destroy_map_view(map_);
    // Owner-thread teardown follows a joined worker, like resource publication.
    map_state_=MapState::empty; map_error_.clear(); ++map_revision_;
    if (!files_) return;
    Unload_All_Shapes();
    files_.reset();
}
void RA2Core::publish_failure(const char* message) noexcept {
    // Even allocation or mutex failure while reporting cannot escape a worker.
    terminal_failure_.store(true, std::memory_order_release);
    try {
        const std::lock_guard lock(progress_mutex_);
        progress_.state = LoadState::failed;
        try { progress_.error = message; }
        catch (...) { progress_.error.clear(); }
    } catch (...) {}
}
void RA2Core::load_resources(std::stop_token stop, const std::string& directory) noexcept {
    Progress p;
    p.state = LoadState::loading;
    auto report = [&] {
        p.mounted.clear();
        p.index_entries = 0;
        if (files_) {
            for (const auto* mix : MixFileClass::MIXes) {
                p.mounted.emplace_back(mix->FileName ? mix->FileName : "");
                const MixHeaderData* entries = nullptr;
                int count = 0;
                if (mix->headers(entries, count)) p.index_entries += static_cast<size_t>(count);
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
                try { progress.current = name; progress.completed = count; progress.total = total; publish(); }
                catch (...) { failure = std::current_exception(); }
            }
        } observation{p, report, stop, {}};
        const auto observe = [](void* data, const char* name, unsigned count, unsigned total) noexcept {
            static_cast<Observation*>(data)->update(name, count, total);
        };
        const auto stopped = [](void* data) noexcept {
            return static_cast<Observation*>(data)->stop.stop_requested();
        };
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

godot::String RA2Core::get_version() const {
    return godot::String::utf8(game::project_version);
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
        bool mounted = false;
        std::string error;
        return game::with_resources(*files_, [](void* context) {
            *static_cast<bool*>(context) = MixFileClass::LoadMapMixes();
        }, &mounted, error) && mounted;
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
        if (progress_.state != LoadState::complete || map_state_==MapState::loading) return {};
    }
    if (worker_.joinable()) worker_.join();
    if (!files_) return {};
    try {
        godot::Ref<RA2INI> result; result.instantiate();
        const auto name = filename.utf8();
        struct Operation { CCINIClass& ini; const char* name; int loaded = 0; } operation{result->ini_, name.get_data()};
        std::string error;
        if (!game::with_resources(*files_, [](void* context) {
            auto& operation = *static_cast<Operation*>(context);
            operation.loaded = operation.ini.LoadFromFile(operation.name);
        }, &operation, error)) throw std::runtime_error(error);
        if (operation.loaded != 1) return {};
        return result;
    } catch (const std::exception& e) {
        ERR_PRINT(godot::String::utf8(e.what()));
        return {};
    }
}

void RA2Core::close_map() {
    cancel_resource_loading(); // Joins either resource or map work before teardown.
    const bool had_map=map_!=nullptr;
    game::destroy_map_view(map_);
    // Drawing now uses SHPReference's original persistent storage. The host
    // releases it after all core consumers stop; queued GPU frames own copies.
    // Keep reference identities/name-cache entries, but not old theater bytes.
    if(had_map) Unload_All_Shapes();
    const std::lock_guard lock(progress_mutex_);
    map_state_=MapState::empty; map_error_.clear(); ++map_revision_;
}
void RA2Core::begin_map_loading(const godot::String& filename) {
    close_map();
    {
        const std::lock_guard lock(progress_mutex_);
        if (progress_.state!=LoadState::complete || !files_) { map_state_=MapState::failed; map_error_="Resources are not ready"; return; }
        map_state_=MapState::loading;
    }
    try {
        const auto utf8=filename.utf8();
        worker_=std::jthread([this,name=std::string(utf8.get_data())](std::stop_token stop) noexcept {
            bool ok=false;
            try {
                ok=game::create_map_view(*files_,map_) && game::load_map_view(*map_,name.data(),static_cast<std::uint32_t>(name.size()));
                std::string error=ok ? "" : map_ ? game::map_view_error(*map_) : "Another map owns the world";
                if (!ok || stop.stop_requested()) game::destroy_map_view(map_);
                const std::lock_guard lock(progress_mutex_);
                map_error_=std::move(error);
                map_state_=stop.stop_requested() ? MapState::cancelled : ok ? MapState::ready : MapState::failed;
            } catch (...) {
                game::destroy_map_view(map_);
                try { const std::lock_guard lock(progress_mutex_); map_state_=MapState::failed; map_error_="Map loading failed"; } catch (...) {}
            }
        });
    } catch (...) {
        const std::lock_guard lock(progress_mutex_); map_state_=MapState::failed; map_error_="Cannot start map loader";
    }
}
godot::Dictionary RA2Core::get_map_status() const {
    const std::lock_guard lock(progress_mutex_);
    constexpr const char* states[]{"empty","loading","ready","failed","cancelled"};
    godot::Dictionary result;
    result["state"]=states[static_cast<unsigned>(map_state_)];
    result["error"]=godot::String::utf8(map_error_.c_str());
    result["revision"]=static_cast<std::int64_t>(map_revision_);
    game::MapViewInfo info{};
    if (map_state_==MapState::ready && map_ && game::get_map_view_info(*map_,info)) {
        result["error"]=godot::String::utf8(game::map_view_error(*map_));
        result["width"]=info.width; result["height"]=info.height; result["theater"]=info.theater;
        result["camera_x"]=info.camera_x; result["camera_y"]=info.camera_y;
        result["camera_revision"]=static_cast<std::int64_t>(info.camera_generation);
        game::MapWorldSnapshot world{};
        if (game::get_map_world_snapshot(*map_, world)) {
            result["presentation_revision"] = static_cast<std::int64_t>(world.presentation_revision);
            result["resource_revision"] = static_cast<std::int64_t>(world.resource_revision);
            result["simulation_tick"] = static_cast<std::int64_t>(world.simulation_tick);
        }
        game::GameViewState ui{};
        if (game::get_game_view_state(*map_,ui)) {
            result["ui_revision"]=static_cast<std::int64_t>(ui.revision);
            result["paused"]=ui.paused; result["pointer_captured"]=ui.pointer_captured;
            result["active_tab"]=ui.active_tab; result["command_bar_expanded"]=ui.command_bar_expanded;
        }
        game::GameViewLayout layout{};
        if (game::get_game_view_layout(*map_,layout)) {
            const auto rect=[](const RectangleStruct& r){return godot::Rect2i(r.X,r.Y,r.Width,r.Height);};
            result["canvas_bounds"]=rect(layout.canvas); result["map_bounds"]=rect(layout.map);
            result["sidebar_bounds"]=rect(layout.sidebar); result["radar_bounds"]=rect(layout.radar);
            result["hud_bounds"]=rect(layout.command_bar);
        }
        result["current_frame"]=ui.current_frame;result["game_speed"]=ui.game_speed;
        result["logic_iterations"]=static_cast<std::int64_t>(ui.logic_iterations);
        result["elapsed_wall_seconds"]=ui.elapsed_wall_seconds;
        game::MapRadarInfo radar{};
        const bool available=game::get_map_radar_info(*map_,radar);
        result["radar_available"]=available && ui.radar_available;
        result["radar_terrain_loaded"]=available;
        if (available) {
            result["radar_x"]=radar.x; result["radar_y"]=radar.y;
            result["radar_width"]=radar.width; result["radar_height"]=radar.height;
            result["radar_frame_valid"]=radar.viewport_valid;
            result["radar_frame_x"]=radar.viewport_x; result["radar_frame_y"]=radar.viewport_y;
            result["radar_frame_width"]=radar.viewport_width; result["radar_frame_height"]=radar.viewport_height;
        }
    }
    return result;
}
bool RA2Core::submit_input(const game::GameInputEvent& event,game::GameInputResult& result) noexcept {
    try { const std::lock_guard lock(progress_mutex_);
        return map_state_==MapState::ready && map_ && game::submit_game_input(*map_,event,result);
    } catch (...) { return false; }
}
bool RA2Core::update_view(double seconds) noexcept {
    try { const std::lock_guard lock(progress_mutex_);
        return map_state_==MapState::ready && map_ && game::update_game_view(*map_,seconds);
    } catch (...) { return false; }
}
game::DrawingStatus RA2Core::advance_map(double seconds,int width,int height,const game::MapDrawingContext& drawing,game::MapDrawStatistics& stats,bool& rendered) noexcept {
    rendered=false;
    try {
        const std::lock_guard lock(progress_mutex_);
        if(map_state_!=MapState::ready||!map_)return game::DrawingStatus::unavailable;
        if(!game::set_game_view_size(*map_,width,height))return game::DrawingStatus::invalid_argument;
        return game::advance_game_view(*map_,seconds,drawing,stats,rendered);
    }catch(...){return game::DrawingStatus::backend_failure;}
}
game::DrawingStatus RA2Core::draw_map(int width,int height,const game::MapDrawingContext& drawing,game::MapDrawStatistics& stats) noexcept {
    try {
        { const std::lock_guard lock(progress_mutex_); if (map_state_!=MapState::ready) return game::DrawingStatus::unavailable; }
        if (worker_.joinable()) worker_.join();
        if (!map_ || !game::set_game_view_size(*map_,width,height)) return game::DrawingStatus::invalid_argument;
        return game::draw_game_view(*map_,drawing,stats);
    } catch (...) { return game::DrawingStatus::backend_failure; }
}
