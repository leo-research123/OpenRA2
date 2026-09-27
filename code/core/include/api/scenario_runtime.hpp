#pragma once
#include "yrpp/GeneralStructures.h"
class CellClass;
class MapClass;
class LightConvertClass;
class ColorScheme;
class TechnoTypeClass;
struct IStream;
struct VolumeStruct;
class ToolTipManager;
class HouseClass;
class HouseTypeClass;
class SessionClass;
class RulesClass;
class CampaignClass;
class MultiMission;
class FileClass;
class Surface;
class CCINIClass;
class LoadProgressManager;
class ConvertClass;
class TacticalClass;
class AbstractTypeClass;
class IsometricTileTypeClass;
class ObjectClass;
class TagClass;
class TagTypeClass;
class ParticleSystemClass;
class AbstractClass;
class SpotlightClass;
template<class TKey, class TValue> class IndexClass;
class MPGameModeClass;
struct ScenarioFlags;
struct NodeNameType;
struct HashIterator;
template<typename T> class DynamicVectorClass;

namespace game {
// Dependencies for the 683AB0 controller. All state is borrowed and remains
// live across callbacks. acquire_file returns an owned FileClass lease, released
// exactly once by release_file (which must not throw); the controller parses
// it with CCINIClass. Failed queries leave outputs unchanged. A successful
// find_theme may return index -1, the original valid "not found" sentinel.
// load_world invokes the world's scenario-loading controller. World object
// initialization is tracked separately from this outer startup sequence.
struct ScenarioStartServices {
    void* context = nullptr;
    SessionClass* session = nullptr;
    DynamicVectorClass<CampaignClass*>* campaigns = nullptr;
    const unsigned char* alternate_campaign = nullptr; // Only byte value 1 selects it.
    const char* alternate_filename = nullptr;
    int* media_check_depth = nullptr;
    const Point2D* current_resolution = nullptr;
    const Point2D* preferred_resolution = nullptr;
    Surface** hidden_surface = nullptr;
    bool* scenario_started = nullptr;
    bool* game_active = nullptr;
    void (*request_disc)(void*, int disc) = nullptr;
    bool (*current_disc)(void*, int timeout, int& disc) = nullptr;
    bool (*mission_has_disc)(void*, MultiMission*, int disc, bool& available) = nullptr;
    bool (*mission_first_disc)(void*, MultiMission*, int& disc) = nullptr;
    bool (*force_disc)(void*) = nullptr;
    void (*hide_cursor)(void*) = nullptr;
    void (*show_cursor)(void*) = nullptr;
    bool (*acquire_file)(void*, const char* filename, FileClass*& file) = nullptr;
    void (*release_file)(void*, FileClass*) = nullptr;
    void (*play_movie)(void*, int movie, int theme, bool, bool, bool) = nullptr;
    void (*stop_theme)(void*, bool immediate) = nullptr;
    bool (*find_theme)(void*, const char* name, int& index) = nullptr;
    void (*play_theme)(void*, int index) = nullptr;
    void (*queue_theme)(void*, int index) = nullptr;
    bool (*load_world)(void*, const char* filename) = nullptr;
    void (*dropship_dialog)(void*) = nullptr;
    void (*resize_display)(void*, int width, int height) = nullptr;
    void (*release_menu_assets)(void*) = nullptr;
    void (*apply_options)(void*) = nullptr;
    void (*present_surface)(void*, Surface*, bool mouse_captured) = nullptr;
    void (*disable_ime)(void*) = nullptr;
    void (*clear_online_state)(void*) = nullptr;
};
// Existing loading-screen objects. get_manager borrows the live singleton;
// detach it before release_manager. Success releases its graphics, whereas
// failure destroys the singleton. No manager ownership passes to Scenario.
struct ScenarioLoadScreenServices {
    void* context = nullptr;
    const int* tournament = nullptr;
    const unsigned* game_id = nullptr;
    bool (*format_game_id)(void*, const wchar_t* format, unsigned id, wchar_t* text, unsigned capacity) = nullptr;
    void (*begin)(void*, double maximum, unsigned char players) = nullptr;
    bool (*get_manager)(void*, LoadProgressManager*&) = nullptr;
    void (*prepare_manager)(void*, LoadProgressManager*) = nullptr;
    void (*attach_manager)(void*, LoadProgressManager*) = nullptr;
    void (*set_side)(void*, int side) = nullptr;
    void (*prepare_surface)(void*, LoadProgressManager*) = nullptr;
    bool (*campaign_palette)(void*, ConvertClass*&) = nullptr;
    void (*load_bar)(void*, const char* filename, ConvertClass*) = nullptr;
    bool (*position)(void*, LoadProgressManager*, Point2D&) = nullptr;
    void (*configure_text)(void*, const Point2D&, const wchar_t*, bool, bool) = nullptr;
    bool (*extent)(void*, LoadProgressManager*, int&) = nullptr;
    void (*set_extent)(void*, int) = nullptr;
    bool (*fraction)(void*, double&) = nullptr;
    void (*set_player_progress)(void*, int player, double progress, double secondary) = nullptr;
    void (*finish)(void*) = nullptr;
    void (*release_manager)(void*, LoadProgressManager*, bool success) = nullptr;
};
// Network/keyboard operations used while loading. drop_connection mutates the
// live player list; progress slots keep their original numbering after a drop.
struct ScenarioNetworkLoadServices {
    void* context = nullptr;
    const bool* delay_before_sync = nullptr;
    // Optional WinSock address group: supply all four fields, or leave all
    // null to skip address setup. peer_ports borrows eight existing slots.
    const unsigned short* peer_ports = nullptr;
    bool (*has_datagram_transport)(void*) = nullptr;
    void (*clear_broadcast_addresses)(void*) = nullptr;
    void (*add_broadcast_address)(void*, const char* ipv4, unsigned short port) = nullptr;
    void (*poll_input)(void*) = nullptr;
    // False: no modem object. True: status contains signal bits, including
    // 0x80 for carrier detect; it is not a sign-extended byte/error code.
    bool (*modem_status)(void*, int& status) = nullptr;
    bool (*player_fraction)(void*, int slot, double&) = nullptr;
    void (*drop_connection)(void*, int house, int error) = nullptr;
    bool (*is_game_host)(void*, HouseClass*, bool&) = nullptr;
    void (*reset_game_host)(void*) = nullptr;
    void (*delay)(void*, unsigned milliseconds, bool) = nullptr;
    void (*finish_online_load)(void*) = nullptr;
};
// 684620 / 686730 dependencies. The two legacy_* callbacks are still
// unported Scenario helpers, not certified world initialization. Other
// callbacks belong to the existing random-map, map, message and House modules.
struct ScenarioWorldServices {
    void* context = nullptr;
    int* initialization_depth = nullptr;
    const RectangleStruct* message_rect = nullptr;
    const ScenarioLoadScreenServices* screen = nullptr;
    const ScenarioNetworkLoadServices* network = nullptr;
    bool (*load_seed)(void*, const char* filename) = nullptr;
    void (*generate_map)(void*) = nullptr;
    void (*legacy_starting_units)(void*, bool official) = nullptr;
    bool (*initialize_world_ini)(void*, CCINIClass&, bool skip_units) = nullptr; // Shared 686B20 controller.
    void (*legacy_finish_world)(void*) = nullptr;
    void (*set_shroud)(void*, bool) = nullptr;
    void (*fade_loading_palette)(void*) = nullptr;
    void (*show_load_error)(void*, const wchar_t* message, const wchar_t* ok) = nullptr;
    void (*configure_messages)(void*, int x, int y, int width) = nullptr;
    void (*finish_house)(void*, HouseClass*) = nullptr;
};
// Dispatch selects an existing owning module, never a substitute object model.
// Rules passes bind shared RulesClass methods; reader return values are ignored
// by 686B20, while exceptions from unavailable modules propagate.
enum class ScenarioRulesPass { countries, general, command_bar, initialize, overlay };
enum class ScenarioObjectReader {
    houses, teams, scripts, task_forces, trigger_types, tags, ai_triggers,
    map, triggers, overlays, terrain, units, aircraft, infantry, buildings, smudges
};
enum class ScenarioInitializationStep {
    clear_swizzler, finish_teams, building_tiles, overlay_bridges,
    voxel_cache, tile_cache_back, tile_cache_front, radar_surface,
    beacon_art, expired_objects, operational_buildings, clear_radar_cells,
    fog, release_map_helpers, redraw_map
};
struct ScenarioInitializeServices {
    void* context = nullptr;
    const int* campaign_difficulty = nullptr;
    ScenarioFlags* special_flags = nullptr;
    unsigned* scenario_crc = nullptr;
    bool* building_read_flag = nullptr;
    const bool* custom_ai = nullptr;
    CCINIClass** rules_ini = nullptr;
    CCINIClass* ai_ini = nullptr;
    CCINIClass* ui_ini = nullptr;
    TacticalClass** tactical = nullptr;
    void (*clear_world)(void*) = nullptr; // Shared 6851F0 controller.
    void (*rules)(void*, ScenarioRulesPass, RulesClass*, CCINIClass*, bool multiplayer) = nullptr;
    void (*read_country)(void*, HouseTypeClass*, CCINIClass*) = nullptr;
    void (*set_map_size)(void*, const RectangleStruct&, bool, bool, bool) = nullptr;
    void (*set_local_size)(void*, const RectangleStruct&) = nullptr;
    void (*prepare_mode)(void*, MPGameModeClass*, bool official) = nullptr;
    void (*start_mode)(void*, MPGameModeClass*, bool official) = nullptr;
    void (*start_special_mode)(void*, bool official) = nullptr;
    void (*mode_ready)(void*, MPGameModeClass*) = nullptr;
    void (*draw_load_screen)(void*, LoadProgressManager*) = nullptr;
    void (*destroy_tactical)(void*, TacticalClass*) = nullptr;
    bool (*create_tactical)(void*, TacticalClass*&) = nullptr;
    void (*tactical_rect)(void*, TacticalClass*, const RectangleStruct&) = nullptr;
    void (*theater)(void*, int index) = nullptr;
    bool (*side)(void*, int index) = nullptr;
    // A successful query may return nullptr for an unknown House name.
    bool (*find_house)(void*, const char* name, HouseClass*&) = nullptr;
    void (*read_objects)(void*, ScenarioObjectReader, CCINIClass*, bool global) = nullptr;
    void (*step)(void*, ScenarioInitializationStep) = nullptr;
    void (*refresh_cell)(void*, CellClass*, int) = nullptr;
    bool (*map_total_value)(void*, bool, int&) = nullptr;
};
enum class ScenarioClearStep {
    stop_audio, stop_lightning, map_objects, tags, tactical_records,
    light_sources, lightning, empulses, veinholes, tile_cache_front,
    tile_cache_back, bombs, kamikazes, aircraft_tracker, planning,
    map_initialize, logic_initialize, campaigns, sidebar_timers, sidebar_objects
};
// Borrow existing collections. Object destructors/Release own removal from
// their live arrays; clearing tag/current-object collections only frees slots.
struct ScenarioClearServices {
    void* context = nullptr;
    DynamicVectorClass<AbstractTypeClass*>* types = nullptr;
    DynamicVectorClass<IsometricTileTypeClass*>* tile_types = nullptr;
    DynamicVectorClass<ObjectClass*>* objects = nullptr;
    DynamicVectorClass<TagTypeClass*>* tag_types = nullptr;
    DynamicVectorClass<TagClass*>* map_tags = nullptr;
    DynamicVectorClass<TagClass*>* logic_tags = nullptr;
    DynamicVectorClass<ObjectClass*>* current_objects = nullptr;
    ParticleSystemClass** particle_system = nullptr;
    const CellStruct* empty_cell = nullptr;
    void (*destroy_world_objects)(void*) = nullptr; // Shared 534450 coordinator.
    void (*step)(void*, ScenarioClearStep) = nullptr;
    void (*drain_particles)(void*, ParticleSystemClass*) = nullptr;
    int (*reset_map_start_positions)(void*) = nullptr;
};
// Each selection refers to an existing engine-owned collection. Query the live
// first item after every destructor/Release, because removal can affect peers.
enum class ScenarioObjectCollection {
    bullets, objects, tags, triggers, tubes, building_lights, overlays,
    particle_systems, waves, factories, sides, teams, houses, animations,
    scripts, radiation_sites, light_sources, empulses, capture_managers,
    disk_lasers, parasites, temporals, airstrikes, spawn_managers, bombs,
    fogged_objects, alpha_shapes, terrain, types
};
enum class ScenarioDestructionStep {
    unload_shapes, expired_objects, electric_bolts, line_trails, lasers,
    map_objects, beacons
};
struct ScenarioDestructionServices {
    void* context = nullptr;
    DynamicVectorClass<AbstractClass*>* notices = nullptr;
    IndexClass<int, AbstractClass*>* target_index = nullptr;
    // Success with a null result means an empty collection; failure preserves
    // result and indicates invalid/unavailable collection storage.
    bool (*first_object)(void*, ScenarioObjectCollection, AbstractClass*&) = nullptr;
    bool (*first_spotlight)(void*, SpotlightClass*&) = nullptr;
    // Spotlight has a nonvirtual destructor. Its owning module unregisters
    // and releases it in this operation; no ownership is retained by Scenario.
    void (*destroy_spotlight)(void*, SpotlightClass*) = nullptr;
    void (*step)(void*, ScenarioDestructionStep) = nullptr;
};
// Synchronous calls into the existing session, event loop and renderer during
// scenario INI loading. armageddon_mode is the original A8ED6B flag. No callback
// reads Scenario fields or substitutes for ScenarioClass::ReadINI.
struct ScenarioIniServices {
    void* context = nullptr;
    const bool* armageddon_mode = nullptr;
    void (*progress)(void*, int percent) = nullptr;
    void (*pump_events)(void*) = nullptr;
    void (*reset_lighting)(void*) = nullptr;
    // Native configuration loading only: parse the original Scenario fields
    // without pumping events, assigning houses or resetting world lighting.
    // armageddon_mode remains required. Default preserves the EXE controller.
    bool fields_only = false;
};
// World-owned original objects. create_house constructs/registers a House in
// the world's registry; the world retains ownership even if scene startup
// later fails. Scenario owns selection/order, while House owns its palette and
// difficulty updates. color_lookup points to nine original player-color slots.
struct ScenarioHouseServices {
    void* context = nullptr;
    SessionClass* session = nullptr;
    DynamicVectorClass<NodeNameType*>* players = nullptr;
    DynamicVectorClass<HouseTypeClass*>* countries = nullptr;
    DynamicVectorClass<HouseClass*>* houses = nullptr;
    HouseClass** current_player = nullptr;
    HouseClass** observer = nullptr;
    RulesClass** rules = nullptr;
    const int* tech_level = nullptr;
    const unsigned char* color_lookup = nullptr;
    bool (*create_house)(void*, HouseTypeClass*, HouseClass*&) = nullptr;
    void (*update_house_color)(void*, HouseClass*) = nullptr;
    void (*update_laser_color)(void*, HouseClass*) = nullptr;
    void (*assign_handicap)(void*, HouseClass*, int difficulty) = nullptr;
};
// Existing audio/input/mouse/tooltip operations used by the pause sequence.
// Double pointers borrow the live singleton slots; callbacks may update them.
struct ScenarioPauseServices {
    void* context = nullptr;
    VolumeStruct** volume = nullptr;
    ToolTipManager** tooltip = nullptr; // The tooltip object is optional.
    const bool* tooltips_enabled = nullptr;
    void (*suspend_audio)(void*) = nullptr;
    void (*resume_audio)(void*) = nullptr;
    void (*set_input_paused)(void*, bool) = nullptr;
    void (*set_tooltip_state)(void*, ToolTipManager*, bool) = nullptr;
    void (*release_mouse_capture)(void*) = nullptr;
    void (*set_cursor)(void*, int index, bool minimap) = nullptr;
    void (*restore_cursor)(void*) = nullptr;
    void (*hide_cursor)(void*) = nullptr;
    void (*show_cursor)(void*) = nullptr;
    void (*render_frame)(void*) = nullptr;
};
// Transport and the world's pointer swizzler are borrowed. A successful
// transfer must read/write the entire requested span. pointer_id identifies
// a TechnoType in the same save transaction; swizzle registers a stable slot
// and may resolve it later, before world loading finishes. No ownership passes.
struct ScenarioStreamServices {
    void* context = nullptr;
    bool (*read)(void*, IStream*, void* data, unsigned bytes) = nullptr;
    bool (*write)(void*, IStream*, const void* data, unsigned bytes) = nullptr;
    bool (*pointer_id)(void*, const TechnoTypeClass*, unsigned& id) = nullptr;
    bool (*swizzle)(void*, unsigned id, TechnoTypeClass** slot) = nullptr;
};
// Borrow original renderer objects and registries. UpdateColors and cell
// traversal execute through their real class interfaces. Palette enumeration
// and sidebar invalidation remain dependencies of their original modules.
struct ScenarioRenderServices {
    void* context = nullptr;
    MapClass* map = nullptr;
    DynamicVectorClass<LightConvertClass*>* light_converts = nullptr;
    DynamicVectorClass<ColorScheme*>* color_schemes = nullptr;
    int* quality = nullptr;
    bool (*next_palette)(void*, HashIterator*, DynamicVectorClass<ColorScheme*>*&) = nullptr;
    int (*palette_scheme_count)(void*) = nullptr;
    void (*redraw_sidebar)(void*, int mode) = nullptr;
};
// Dependencies owned by the existing session, trigger and string-table
// modules. This record stores no copy of the Scenario or of world objects.
// All pointers are borrowed for the synchronous with_scenario_runtime call.
struct ScenarioRuntimeServices {
    void* context = nullptr;
    int (*session_mode)(void*) noexcept = nullptr;
    void (*variable_changed)(void*, bool global, int index) = nullptr;
    bool (*stringtable)(void*, const char* label, int source_line, const wchar_t*& result) = nullptr;
    // The existing map's GetCellAt semantics, including its InvalidCell object.
    bool (*cell_at)(void*, const CellStruct&, CellClass*&) = nullptr;
    const ScenarioRenderServices* render = nullptr;
    const ScenarioStreamServices* stream = nullptr;
    const ScenarioPauseServices* pause = nullptr;
    const ScenarioHouseServices* houses = nullptr;
    const ScenarioIniServices* ini = nullptr;
    const ScenarioStartServices* start = nullptr;
    const ScenarioWorldServices* world = nullptr;
    const ScenarioInitializeServices* initialize = nullptr;
    const ScenarioClearServices* clear = nullptr;
    const ScenarioDestructionServices* destruction = nullptr;
};

// Missing callbacks return false without invoking operation. Exceptions from
// operation propagate after restoring the previous services. The owner must
// serialize access to the original global objects and keep them alive.
bool with_scenario_runtime(const ScenarioRuntimeServices& services,
    void (*operation)(void*), void* context);
}
