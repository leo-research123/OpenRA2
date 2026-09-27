#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "api/filesystem.hpp"
#include "api/map_view.hpp"
#include "api/map_objects.hpp"
#include <godot_cpp/variant/array.hpp>
#include "bridge/ra2_ini.hpp"
#include <mutex>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class RA2Core : public godot::RefCounted {
    GDCLASS(RA2Core, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    ~RA2Core() override;
    godot::String get_version() const;
    void begin_resource_loading(const godot::String& directory);
    godot::Dictionary get_resource_progress() const;
    void cancel_resource_loading();
    godot::Ref<RA2INI> load_ini(const godot::String& filename);
    // Owner-thread, idle-map operation. Mounts the original map packages for
    // menu discovery; failures return false and no exception crosses Godot.
    bool prepare_map_resources() noexcept;
    void begin_map_loading(const godot::String& filename);
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
    bool set_map_building_enabled(std::int64_t world, std::int64_t id, bool enabled);
    godot::Dictionary harvest_map_resource(int x, int y, int requested);
    bool submit_input(const game::GameInputEvent&,game::GameInputResult&) noexcept;
    bool update_view(double seconds) noexcept;
    game::DrawingStatus advance_map(double seconds,int width,int height,const game::MapDrawingContext&,game::MapDrawStatistics&,bool& rendered) noexcept;
    game::DrawingStatus draw_map(int width,int height,const game::MapDrawingContext&,game::MapDrawStatistics&) noexcept;
private:
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
    MapState map_state_=MapState::empty;
    std::string map_error_;
    std::uint64_t map_revision_=0;
    game::MapViewHandle* map_=nullptr;
    // Platform service lifetime only. All package slots/arrays/list live in YRpp.
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> files_{nullptr, game::destroy_resources};
    std::jthread worker_;
};
