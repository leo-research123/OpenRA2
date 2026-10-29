#pragma once
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/MPGameModeClass.h"
#include <memory>
#include "game_loop.hpp"
#include "tooltip_platform.hpp"
#include "yrpp/VocClass.h"
#include "debug/debug_state.hpp"
#include "ui_render_state.hpp"

namespace game {
class UiResources;
class MapWorld;
// Session ownership only. Map/Cell state remains in the stable original root.
class MapViewHandle {
public:
    ResourceHandle* resources;
    std::unique_ptr<MPBattleClass> battle_mode;
    NodeNameType local_player{};
    ScenarioClass scenario;
    // The Tactical object lives on the heap behind TacticalClass::Instance, as in
    // the original: ClearWorld and InitializeWorldINI replace it, and the handle
    // deletes whichever one is current when it goes away.
    InputManagerClass keyboard;
    // Own the original Rules object only when startup has not supplied one.
    std::unique_ptr<RulesClass> rules;
    // Raw device pointer and scheduling remainder, no hit/capture/UI state.
    Point2D pointer{};
    bool pointer_inside = false;
    bool options_paused = false;
    VolumeStruct detached_pause_volume{};
    CCToolTip tooltips;
    ToolTipPlatform tooltip_platform;
    GameLoopState loop;
    MissionTransitionInfo transition;
    short next_stage = 0;
    int next_campaign = -1;
    bool transition_prepared = false;
    bool continuation_loading = false;
    int campaign_difficulty = -1;
    char loaded_filename[260]{};
    DebugState debug;
    std::uint64_t ui_generation = 0;
    unsigned operation_depth = 0;
    MapViewState state = MapViewState::empty;
    std::uint64_t generation = 0;
    std::uint64_t camera_generation = 0;
    char error[1024]{};
    bool terrain_loaded = false;
    bool shapes_prepared = false;  // shape sequences announced to the host for this map
    std::unique_ptr<UiResources> ui_resources;
    UiRenderState ui_render;
    std::unique_ptr<MapWorld> world;
    explicit MapViewHandle(ResourceHandle& source);
    ~MapViewHandle();
    MapViewHandle(const MapViewHandle&) = delete;
    MapViewHandle& operator=(const MapViewHandle&) = delete;
};
// Internal loading/drawing operation scope; tests explicitly opt into access.
// Original global Scenario and all borrowed resource/service bindings are
// restored on both success and failure. Callbacks may not re-enter destruction.
bool with_map_view(MapViewHandle&, void (*operation)(void*), void*) noexcept;
bool initialize_empty_map_view(MapViewHandle&, const RectangleStruct&, char level) noexcept;
// Once per map, on its first drawn frame: announces the images the scene can draw to a host
// with a shape_sequence callback (shape_prewarm.cpp). Never fails the frame.
void prepare_map_shapes(MapViewHandle&, const MapDrawingContext&) noexcept;
}
