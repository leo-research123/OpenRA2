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
class CCINIClass;
class LoadProgressManager;
class ConvertClass;
class TacticalClass;
class MPGameModeClass;
struct ScenarioFlags;
struct NodeNameType;
struct HashIterator;
template <typename T>
class DynamicVectorClass;

namespace game {
// Dependencies for the 683AB0 controller. All state is borrowed and remains
// live across callbacks. acquire_file returns an owned FileClass lease, released
// exactly once by release_file (which must not throw); the controller parses
// it with CCINIClass. Failed queries leave outputs unchanged. A successful
// find_theme may return index -1, the original valid "not found" sentinel.
// load_world invokes the world's scenario-loading controller. World object
// initialization is tracked separately from this outer startup sequence.
// show_cursor must not throw: it also restores the cursor during unwinding.
struct ScenarioStartServices {
    void* context = nullptr;
    int* media_check_depth = nullptr;
    const Point2D* current_resolution = nullptr;
    const Point2D* preferred_resolution = nullptr;
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
    void (*disable_ime)(void*) = nullptr;
    void (*clear_online_state)(void*) = nullptr;
};
// Existing loading-screen objects. get_manager borrows the live singleton;
// detach it before release_manager. Success releases its graphics, whereas
// failure destroys the singleton. No manager ownership passes to Scenario.
// attach_manager and release_manager must not throw during failure cleanup.
struct ScenarioLoadScreenServices {
    void* context = nullptr;
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
    void (*finish_online_load)(void*) = nullptr;
};
// 684620 / 686730 operation boundaries. Native hosts bind the original
// Scenario controllers and House::RecordStartingUnits at the corresponding
// boundaries. Starting-unit generation (686890) remains unported.
struct ScenarioWorldServices {
    void* context = nullptr;
    const RectangleStruct* message_rect = nullptr;
    const ScenarioLoadScreenServices* screen = nullptr;    // Optional for local loading.
    const ScenarioNetworkLoadServices* network = nullptr;  // Required for network modes.
    bool (*load_seed)(void*, const char* filename) = nullptr;
    void (*generate_map)(void*) = nullptr;
    void (*legacy_starting_units)(void*, bool official) = nullptr;
    bool (*initialize_world_ini)(void*, CCINIClass&, bool skip_units) = nullptr;  // Shared 686B20 controller.
    void (*legacy_finish_world)(void*) = nullptr;
    void (*set_shroud)(void*, bool) = nullptr;
    void (*fade_loading_palette)(void*) = nullptr;
    void (*show_load_error)(void*, const wchar_t* message, const wchar_t* ok) = nullptr;
    void (*configure_messages)(void*, int x, int y, int width) = nullptr;
    void (*finish_house)(void*, HouseClass*) = nullptr;
    // Original UI activation. Native renderers without the widget may leave it unset.
    void (*activate_sidebar)(void*, int control) = nullptr;
};
// 686B20 calls the ported originals directly. The services below stand for
// originals that are not ported yet or that belong to the host. Reader return
// values are ignored by 686B20, while exceptions from unavailable modules propagate.
// The map reader (0x4ACE70).
enum class ScenarioObjectReader { map };
// Unported calls: the laser fence pass (0x452D40), fog (0x5866C0) and the
// radar redraw (0x657CE0).
enum class ScenarioInitializationStep { operational_buildings, fog, redraw_map };
struct ScenarioInitializeServices {
    void* context = nullptr;
    // 0x00653F50 (CreateEmptyMap) has no native body yet.
    void (*set_map_size)(void*, const RectangleStruct&, bool, bool, bool) = nullptr;
    void (*prepare_mode)(void*, MPGameModeClass*, bool official) = nullptr;
    void (*start_mode)(void*, MPGameModeClass*, bool official) = nullptr;
    void (*start_special_mode)(void*, bool official) = nullptr;
    void (*mode_ready)(void*, MPGameModeClass*) = nullptr;
    void (*draw_load_screen)(void*, LoadProgressManager*) = nullptr;
    void (*tactical_rect)(void*, TacticalClass*, const RectangleStruct&) = nullptr;
    void (*theater)(void*, int index) = nullptr;
    bool (*side)(void*, int index) = nullptr;
    void (*read_objects)(void*, ScenarioObjectReader, CCINIClass*) = nullptr;
    void (*step)(void*, ScenarioInitializationStep) = nullptr;
};
// ScenarioClass::ClearWorld (0x6851F0) calls the originals directly. These
// two belong to the host: releasing the playing sound events (0x750FA0) and
// rebuilding the campaign list from the BATTLE*.INI files on disk (0x52CB90).
struct ScenarioClearServices {
    void* context = nullptr;
    void (*stop_audio)(void*) = nullptr;
    void (*init_campaigns)(void*) = nullptr;
};
// Host event loop and renderer operations during scenario INI loading.
// Session mode and ArmageddonMode are read from the original globals.
struct ScenarioIniServices {
    void* context = nullptr;
    void (*progress)(void*, int percent) = nullptr;
    void (*pump_events)(void*) = nullptr;
    void (*reset_lighting)(void*) = nullptr;
};
// Existing audio/input/mouse/tooltip operations used by the pause sequence.
// Double pointers borrow the live singleton slots; callbacks may update them.
struct ScenarioPauseServices {
    void* context = nullptr;
    VolumeStruct** volume = nullptr;
    ToolTipManager** tooltip = nullptr;  // The tooltip object is optional.
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
// Custom-palette enumeration and sidebar invalidation remain module operations.
// Map, light converts, color schemes and quality use their original globals.
struct ScenarioRenderServices {
    void* context = nullptr;
    bool (*next_palette)(void*, HashIterator*, DynamicVectorClass<ColorScheme*>*&) = nullptr;
    int (*palette_scheme_count)(void*) = nullptr;
    void (*redraw_sidebar)(void*, int mode) = nullptr;
};
// Host/module operations only. State is authoritative in SessionClass::Instance,
// HouseClass, MapClass and the other original globals, including in nested scopes.
// All pointers are borrowed for the synchronous with_scenario_runtime call.
struct ScenarioRuntimeServices {
    void* context = nullptr;
    void (*variable_changed)(void*, bool global, int index) = nullptr;
    bool (*stringtable)(void*, const char* label, int source_line, const wchar_t*& result) = nullptr;
    const ScenarioRenderServices* render = nullptr;
    const ScenarioPauseServices* pause = nullptr;
    const ScenarioIniServices* ini = nullptr;
    const ScenarioStartServices* start = nullptr;
    const ScenarioWorldServices* world = nullptr;
    const ScenarioInitializeServices* initialize = nullptr;
    const ScenarioClearServices* clear = nullptr;
};

// Missing callbacks return false without invoking operation. Exceptions from
// operation propagate after restoring the previous services. The owner must
// serialize access to the original global objects and keep them alive.
bool with_scenario_runtime(const ScenarioRuntimeServices& services, void (*operation)(void*), void* context);
}
