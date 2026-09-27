#pragma once
#include <cstdint>
#include "api/type_drawing.hpp"
struct BytePalette;

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
    std::uint64_t presentation_revision=0,resource_revision=0,simulation_tick=0;
};
struct MapDrawStatistics { std::uint32_t visited=0, drawn=0, skipped=0; };
// Read-only diagnostics of original UI state. A host must not build controls
// or interpret input using this snapshot; production draws the core's frame.
struct GameViewLayout {
    RectangleStruct canvas{}, map{}, sidebar{}, radar{}, command_bar{};
    Point2D cameo_origin{}, cameo_pitch{}, scroll_origin{};
    int side_index = 0;
    int visible_cameos = 0;
    int original_cameos = 0;
};
enum class GameInputKind : std::uint32_t { pointer_move, pointer_button, key, wheel, focus_lost, pointer_leave, focus_gained };
// Whole-canvas logical pixels. Buttons use 1=left, 2=right, 3=middle;
// keys use original virtual-key numbers. Device modifiers are shift=1,ctrl=2,alt=4.
struct GameInputEvent {
    GameInputKind kind{};
    int x=0,y=0;
    std::uint32_t code=0,modifiers=0;
    bool pressed=false;
    int wheel=0;
};
struct GameInputResult {
    bool consumed=false;
    bool warp_pointer=false;
    int pointer_x=0,pointer_y=0;
};
struct GameViewState {
    std::uint64_t revision=0;
    bool paused=false,radar_available=false,pointer_captured=false;
    int active_tab=0;
    bool command_bar_expanded=false;
    int current_frame=0,game_speed=0;
    std::uint64_t logic_iterations=0;
    double elapsed_wall_seconds=0;
    bool focused=true;
};
// Events are consumed synchronously in order; no game semantics in the host.
// Tick duration is elapsed device time. Focus loss clears held input/capture.
bool submit_game_input(MapViewHandle&,const GameInputEvent&,GameInputResult&) noexcept;
bool update_game_view(MapViewHandle&,double seconds) noexcept;
bool get_game_view_state(const MapViewHandle&,GameViewState&) noexcept;
struct MapRadarInfo {
    int panel_width, panel_height;
    int x, y, width, height;
    int raw_width, raw_height;
    int viewport_x, viewport_y, viewport_width, viewport_height;
    bool viewport_valid;
};
struct MapDrawingContext {
    TypeDrawingContext types{};
    int lighting_quality=2;
    // Original unlit ConvertClass palette (no LightConvert tint arithmetic).
    DrawingStatus (*plain_palette)(void*,const BytePalette&,
        const DrawingPaletteHandle*&) noexcept=nullptr;
    // Resolve a backend-owned palette from original normalized/quantized tint.
    // The PAL and returned identity are borrowed synchronously, like type draws.
    DrawingStatus (*terrain_palette)(void*, const BytePalette&, int red, int green,
        int blue, int shade_count, const DrawingPaletteHandle*&) noexcept=nullptr;
    // Original ConvertClass shade table for global animation/resource palettes.
    // Neutral LightConvert arithmetic is different even without a color tint.
    DrawingStatus (*shape_palette)(void*,const BytePalette&,int shade_count,
        const DrawingPaletteHandle*&) noexcept=nullptr;
    // Original house ColorScheme converter, including its unlit color mask.
    // Borrowed synchronously; failures return status and never throw.
    DrawingStatus (*color_scheme_palette)(void*,const BytePalette&,int shade_count,
        const DrawingPaletteHandle*&) noexcept=nullptr;
};
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
DrawingStatus advance_game_view(MapViewHandle&,double seconds,const MapDrawingContext&,
    MapDrawStatistics&,bool& rendered) noexcept;
void destroy_map_view(MapViewHandle*& view) noexcept;
bool get_map_view_info(const MapViewHandle& view, MapViewInfo& output) noexcept;
// Borrowed until the next operation on this live handle; never null.
const char* map_view_error(const MapViewHandle& view) noexcept;
}
