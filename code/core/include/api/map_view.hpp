#pragma once
#include <cstdint>
#include "api/type_drawing.hpp"
struct BytePalette;
class ConvertClass;

namespace game {
class ResourceHandle;
class MapViewHandle;
enum class MapViewState : std::uint32_t { empty, ready, failed };
struct MapViewInfo {
    MapViewState state;
    std::uint64_t generation;
    int width;
    int height;
    int slot_capacity;
    bool terrain_loaded;
    int theater;
    int visible_x, visible_y, visible_width, visible_height;
    int viewport_width, viewport_height;
    int camera_x, camera_y;
    std::uint64_t camera_generation;
    std::uint64_t presentation_revision = 0, resource_revision = 0, simulation_tick = 0;
};
struct MapDrawStatistics {
    std::uint32_t visited = 0, drawn = 0, skipped = 0;
};
struct GameCursorFrame {
    bool visible = false;
    int width = 0, height = 0, hotspot_x = 0, hotspot_y = 0;
    int x = 0, y = 0, index = 0, frame = 0;
    std::uint64_t revision = 0;
};
// The original cursor's current cropped frame, RGBA8 in row-major byte order.
// A null buffer with zero capacity queries metadata only. A provided buffer
// must hold width*height*4 bytes and may not alias core storage. Palette edits
// change revision even if shape/frame are unchanged. No exceptions escape.
bool read_game_cursor(MapViewHandle&, GameCursorFrame&, std::uint8_t* rgba = nullptr,
                      std::uint32_t capacity = 0) noexcept;
// Read-only diagnostics of original UI state. A host must not build controls
// or interpret input using this snapshot; production draws the core's frame.
struct GameViewLayout {
    RectangleStruct canvas{}, map{}, sidebar{}, radar{}, command_bar{};
    Point2D cameo_origin{}, cameo_pitch{}, scroll_origin{};
    int side_index = 0;
    int visible_cameos = 0;
    int original_cameos = 0;
};
enum class GameInputKind : std::uint32_t {
    pointer_move,
    pointer_button,
    key,
    wheel,
    focus_lost,
    pointer_leave,
    focus_gained
};
// Whole-canvas logical pixels. Buttons use 1=left, 2=right, 3=middle;
// keys use original virtual-key numbers. Device modifiers are shift=1,ctrl=2,alt=4.
struct GameInputEvent {
    GameInputKind kind{};
    int x = 0, y = 0;
    std::uint32_t code = 0, modifiers = 0;
    bool pressed = false;
    int wheel = 0;
};
struct GameInputResult {
    bool consumed = false;
    bool warp_pointer = false;
    int pointer_x = 0, pointer_y = 0;
};
enum class GameResult : std::uint32_t { in_progress, victory, defeat };
enum class MissionTransitionState : std::uint32_t { none, next, retry, complete, unavailable, loading, failed };
struct MissionTransitionInfo {
    std::uint64_t id = 0;
    GameResult result = GameResult::in_progress;
    MissionTransitionState state = MissionTransitionState::none;
    char filename[260]{};
    char error[512]{};
};
// Queries are read-only. begin validates the current result id exactly once;
// load runs after begin on the serialized loader thread, with render/input stopped.
// A failed load retains its prepared handoff and can be retried with the same id.
// No exception crosses these boundaries; stale ids and reentry preserve state.
bool get_mission_transition(const MapViewHandle&, MissionTransitionInfo&) noexcept;
bool begin_mission_transition(MapViewHandle&, std::uint64_t id) noexcept;
bool load_mission_transition(MapViewHandle&) noexcept;
// Report a host loader failure before execution, retaining the prepared request.
void fail_mission_transition(MapViewHandle&, const char* message) noexcept;
struct GameViewState {
    std::uint64_t revision = 0;
    bool paused = false, radar_available = false, pointer_captured = false;
    int active_tab = 0;
    bool command_bar_expanded = false;
    int current_frame = 0, game_speed = 0;
    std::uint64_t logic_iterations = 0;
    double elapsed_wall_seconds = 0;
    bool focused = true;
    int speed_level = 5, target_logic_fps = 0;
    unsigned batch_steps = 0;
    bool batch_limited = false;
    int special_dialog = 0;
    GameResult result = GameResult::in_progress;
};
// Names are borrowed engine strings. Bindings use original WWKey values; zero
// unbinds a command. Bulk replacement is atomic and rejects duplicate keys.
// No exceptions escape. Hosts serialize access with other core operations.
struct PlayerHotkeyBinding {
    const char* command = nullptr;
    const wchar_t* name = nullptr;
    unsigned key = 0, default_key = 0;
};
unsigned player_hotkey_count() noexcept;
bool read_player_hotkey(unsigned index, PlayerHotkeyBinding& output) noexcept;
bool set_player_hotkeys(const unsigned* keys, unsigned count) noexcept;
// Events are consumed synchronously in order; no game semantics in the host.
// Tick duration is elapsed device time. Focus loss clears held input/capture.
bool submit_game_input(MapViewHandle&, const GameInputEvent&, GameInputResult&) noexcept;
bool update_game_view(MapViewHandle&, double seconds) noexcept;
bool get_game_view_state(const MapViewHandle&, GameViewState&) noexcept;
// Increasing levels: 1..6 original timed speeds, 7=120, 8=240 logic frames
// per second, 9=unlimited. Default 5. Invalid/reentrant calls preserve state.
// May be set before loading; reloads preserve the preference. No exceptions
// cross these boundaries. The host serializes all access to the live handle.
bool set_game_speed_level(MapViewHandle&, int level) noexcept;
bool get_game_speed_level(const MapViewHandle&, int& level) noexcept;
// Focus changes do not pause gameplay or reset the simulation schedule.
// Explicit host pause is separate from Scenario's nested/timed pause; its
// transitions discard scheduling debt without advancing gameplay or wall time.
// Also synchronizes the installed movie session: host pause stops media,
// while focus loss and Scenario logic pause leave movie playback running.
bool set_game_view_activity(MapViewHandle&, bool focused, bool paused) noexcept;
bool update_game_view_batch(MapViewHandle&, double seconds) noexcept;
struct MapRadarInfo {
    int panel_width, panel_height;
    int x, y, width, height;
    int raw_width, raw_height;
    int viewport_x, viewport_y, viewport_width, viewport_height;
    bool viewport_valid;
};
struct MapDrawingContext;
// One independent world per process. The caller serializes calls, owns the
// handle uniquely, and destroys it before its borrowed ResourceHandle. Output
// must be null. Creation never replaces an existing active game/host world.
// All failures stay on this side of the boundary; no exceptions propagate.
bool create_map_view(ResourceHandle& resources, MapViewHandle*& output) noexcept;
// Borrows filename_length UTF-8 bytes; no embedded NUL. Reads through the
// resource environment. Invalid arguments leave the current world intact;
// after loading begins, failure unpublishes and frees the prior map.
// This publishes terrain; entity/production simulation has separate stages.
bool load_map_view(MapViewHandle& view, const char* filename, std::uint32_t filename_length) noexcept;
// A copied frontend request, translated into the original Session/NodeName records.
// Slot 0 is the local human; remaining slots are open (-1) or AI (0 hard,
// 1 normal, 2 easy). Country/color/start use -2 for random; team -1 is unallied.
struct SkirmishPlayer {
    int kind = -1, country = -2, color = -2, start = -2, team = -1;
};
// Thirty players plus Neutral and Special fill the 32-bit per-House masks.
// Colors 0..7 are the original multiplayer colors; 8..29 are native additions.
constexpr int SkirmishSlotCount = 30;
constexpr int SkirmishColorCount = 30;
struct SkirmishOptions {
    SkirmishPlayer players[SkirmishSlotCount]{};
    int money = 10000, unit_count = 10;
    int game_speed = 1; // Original delay: 0 fastest, 6 slowest.
    std::uint32_t seed = 0;
    bool short_game = true, super_weapons = true, build_off_ally = false;
    bool mcv_redeploy = true, crates = false, bridge_destruction = true;
};
// Starts ordinary Battle only. Validates player choices and map capacity before
// original scenario initialization. No exceptions escape. Invalid scalar choices
// preserve the current world; resource/map errors follow load_map_view rollback.
bool load_skirmish_view(MapViewHandle&, const char* filename, std::uint32_t filename_length,
                        const SkirmishOptions&) noexcept;
// Campaign handicap: 0 easy, 1 normal, 2 hard (distinct from AI slot labels).
// Configure an empty view before loading. The choice survives mission retries
// and transitions on this handle. Invalid/live/reentrant calls preserve state.
// No exceptions escape; multiplayer settings are unaffected.
bool set_campaign_difficulty(MapViewHandle&, int difficulty) noexcept;
// Local map output pixels. The first size focuses the Scenario home cell;
// resizing preserves the center where the new limits permit it. Camera state
// remains in the real Tactical.
bool set_map_viewport(MapViewHandle&, int width, int height) noexcept;
// Full logical game canvas; original classes own the subdivision. Native 1x
// right sidebar. Invalid size/state leaves the current layout intact.
bool set_game_view_size(MapViewHandle&, int width, int height) noexcept;
bool get_game_view_layout(MapViewHandle&, GameViewLayout& output) noexcept;
// Projected map pixels, before subtracting the camera. The core clamps against
// the original visible-map limits. Oversized host viewports center the map on
// that axis. Failed calls leave camera state unchanged; outputs remain original
// Tactical state, never a separate host camera. No exceptions cross this API.
bool center_map_view(MapViewHandle&, int x, int y) noexcept;
bool scroll_map_view(MapViewHandle&, int delta_x, int delta_y) noexcept;
// Terrain radar only. Original RGB storage/projection belong to RadarClass;
// the host receives geometry and copies a presentation image in RGB565.
// Dimensions/clicks are in native 140x108 panel pixels, including letterbox.
// Missing radar, invalid capacities/coordinates, or re-entry return false.
// Copy output must not alias core storage. No exceptions cross these APIs.
bool get_map_radar_info(MapViewHandle&, MapRadarInfo&) noexcept;
bool copy_map_radar_pixels(MapViewHandle&, std::uint16_t* output, std::uint32_t pixel_capacity) noexcept;
bool center_map_from_radar(MapViewHandle&, int x, int y) noexcept;
// Whole-target drawing: caller begins with cleared color, Z=0xFFFF and
// ABuffer=127. Raster shroud/fog/alpha_shape writes precede color consumers.
// Failed submissions must be discarded; the next call rebuilds the full frame.
DrawingStatus draw_map_view(MapViewHandle&, const MapDrawingContext&, MapDrawStatistics&) noexcept;
// Complete logical canvas, original class UI drawing plus the tactical map.
// Resource/backend failure is explicit; no host-side UI reconstruction.
DrawingStatus draw_game_view(MapViewHandle&, const MapDrawingContext&, MapDrawStatistics&) noexcept;
// Host event-loop entry: input, render, Logic.Update, CurrentFrame++, original wait.
// At most one iteration per call; stalls slow the game, with no catch-up debt.
// rendered=false while waiting. Rendering is optional through update_game_view.
DrawingStatus advance_game_view(MapViewHandle&, double seconds, const MapDrawingContext&, MapDrawStatistics&,
                                bool& rendered) noexcept;
// Device-time driver with bounded batches for levels 7..9. Retains complete
// original iterations, emits at most one frame, and never accumulates stall
// debt. System timers use wall time; frame timers use each actual iteration.
DrawingStatus advance_game_view_batch(MapViewHandle&, double seconds, const MapDrawingContext&, MapDrawStatistics&,
                                      bool& rendered) noexcept;
void destroy_map_view(MapViewHandle*& view) noexcept;
bool get_map_view_info(const MapViewHandle& view, MapViewInfo& output) noexcept;
// Borrowed until the next operation on this live handle; never null.
const char* map_view_error(const MapViewHandle& view) noexcept;
}
