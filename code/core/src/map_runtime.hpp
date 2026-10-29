#pragma once
#include "yrpp/GeneralStructures.h"
class PixelFXClass;
class TabClass;

namespace game {
// Dependencies of the original map modules; the map remains the state owner.
// Values are borrowed for the synchronous scope. The calibrated YR height
// scale is in [0, 0.5]; zero also preserves the pre-initialization EXE state.
// A missing viewport is unavailable, never a guessed window resolution.
struct MapRuntimeServices {
    const double* height_scale = nullptr;
    const RectangleStruct* view_bounds = nullptr;
    const bool* count_light_convert_references = nullptr;
    void (*destroy_pixel_fx)(PixelFXClass*) noexcept = nullptr;
    // GUI notification delivery is optional until the host connects its tabs.
    bool (*tab_notice)(TabClass*, DWORD) noexcept = nullptr;
    // Optional owner of the tile light-convert cache (544FF0). The standalone
    // fresh-cell path has no such cache; a renderer creating one must bind it.
    void (*release_cell_lighting)() noexcept = nullptr;
    // Optional filesystem boundary for catalog verification/original binding.
    // Without it, the original CCFileClass chain resolves the filename.
    bool (*tile_file_exists)(const char*) noexcept = nullptr;
    // Native PALETTE.PAL backing for original UI action-line color indexes.
    const BytePalette* action_line_palette = nullptr;
    // Drawing uses TacticalRect (0x00886FA0); CoordsToClient uses the separate
    // viewport above (0x00B0CE28). Window presence is not keyboard focus.
    const RectangleStruct* drawing_bounds = nullptr;
    const bool* debug_map = nullptr;
    bool (*has_window)() noexcept = nullptr;
};
const MapRuntimeServices& default_map_runtime() noexcept;
const MapRuntimeServices& map_runtime() noexcept;
// Callbacks must not throw across their module boundary. A same-runtime
// exception is caught here and reports failure after restoring the scope.
bool with_map_runtime(const MapRuntimeServices&, void (*operation)(void*), void*) noexcept;
bool map_view_bounds(RectangleStruct& output) noexcept;
}
