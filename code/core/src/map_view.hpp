#pragma once
#include "api/map_view.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include <memory>
#include "game_loop.hpp"
#include "tooltip_platform.hpp"

namespace game {
class UiResources;
class MapWorld;
// Session ownership only. Map/Cell state remains in the stable original root.
class MapViewHandle {
public:
    ResourceHandle* resources;
    ScenarioClass scenario;
    TacticalClass tactical;
    InputManagerClass keyboard;
    SessionClass session{};
    // Own the original Rules object only when startup has not supplied one.
    std::unique_ptr<RulesClass> rules;
    // Raw device pointer and scheduling remainder, no hit/capture/UI state.
    Point2D pointer{};
    bool pointer_inside=false;
    CCToolTip tooltips;
    ToolTipPlatform tooltip_platform;
    GameLoopState loop;
    std::uint64_t ui_generation=0;
    unsigned operation_depth = 0;
    MapViewState state = MapViewState::empty;
    std::uint64_t generation = 0;
    std::uint64_t camera_generation = 0;
    char error[1024]{};
    int animation_baseline;
    bool terrain_loaded = false;
    std::unique_ptr<UiResources> ui_resources;
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
}
