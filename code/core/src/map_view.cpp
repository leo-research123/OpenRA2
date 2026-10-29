#include "background_invalidation.hpp"
#include "verify/draw_coverage.hpp"
#include "presentation/recorder.hpp"
#include "render_phase.hpp"
#include "presentation/ui/screen.hpp"
#include "visibility_grid.hpp"
#include "audio_session.hpp"
#include "com_error.hpp"
#include "yrpp/ThemeClass.h"
#include "yrpp/VoxClass.h"
#include "map_view.hpp"
#include "yrpp/EvadeClass.h"
#include <atomic>
#include "screen_effect.hpp"
#include "sidebar_movie.hpp"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/MessageListClass.h"
#include "tactical_drawing.hpp"
#include "type_drawing.hpp"
#include "api/drawing_diagnostics.hpp"
#include "map_world.hpp"
#include "map_configuration.hpp"
#include "api/filesystem.hpp"
#include "yrpp/SessionClass.h"
#include "scenario_runtime.hpp"
#include "runtime_diagnostics.hpp"
#include "map_world_internal.hpp"
#include "clock.hpp"
#include "game_ui_runtime.hpp"
#include "player_commands.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/TagClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Surface.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/Audio.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/ConvertClass.h"
#include <stdexcept>
#include "yrpp/Theater.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/HouseTypeClass.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <algorithm>
#include <climits>
#include <cmath>

namespace game {
namespace {
MapViewHandle* active = nullptr;
bool effective_audio_paused(const MapViewHandle& view) noexcept {
    return view.loop.host_paused || view.scenario.unknown_62C != 0;
}
// Bind the original input admission globals for a live native map canvas,
// including a terrain-only view. Restore them before leaving the host call.
struct InputScope {
    bool game_active = Game::IsActive, tactical_active = Unsorted::TacticalActive;
    explicit InputScope(bool active) {
        Game::IsActive = active;
        Unsorted::TacticalActive = active;
    }
    ~InputScope() {
        Game::IsActive = game_active;
        Unsorted::TacticalActive = tactical_active;
    }
};
// The scenario services a native view binds for each operation. Original
// registries are bound directly; the callbacks are the parts this host owns:
// its Tactical object, theater resources, the map reader and audio.
struct ViewScenarioServices {
    ScenarioRenderServices rendering{};
    ScenarioIniServices ini{};
    ScenarioStartServices start{};
    ScenarioWorldServices world{};
    ScenarioInitializeServices initialize{};
    ScenarioClearServices clear{};
    RectangleStruct message_rect{};
    Point2D resolution{};
    int media_check_depth = 0;
    ViewScenarioServices(MapViewHandle& view, ScenarioRuntimeServices& services) {
        // 0x0053AD00 walks the custom-palette registry and updates the first
        // GetNumberOfSchemes entries of each palette's scheme vector.
        rendering.next_palette = [](void*, HashIterator* iterator, DynamicVectorClass<ColorScheme*>*& schemes) {
            schemes = ColorScheme::GetPaletteSchemesFromIterator(iterator);
            return schemes != nullptr;
        };
        rendering.palette_scheme_count = [](void*) { return ColorScheme::GetNumberOfSchemes(); };
        rendering.redraw_sidebar = [](void*, int mode) { SidebarClass::Instance.MarkNeedsRedraw(mode); };
        services.render = &rendering;
        // No loading screen: progress and the event pump have nothing to show.
        ini.progress = [](void*, int) {};
        ini.pump_events = [](void*) {};
        ini.reset_lighting = [](void*) {};
        services.ini = &ini;
        start.context = &view;
        start.media_check_depth = &media_check_depth;
        resolution = {DSurface::ViewBounds.Width, DSurface::ViewBounds.Height};
        start.current_resolution = start.preferred_resolution = &resolution;
        // Mounted resources replace the original physical-disc selection.
        start.request_disc = [](void*, int) {};
        start.current_disc = [](void*, int, int& disc) {
            disc = -1;
            return true;
        };
        start.mission_has_disc = [](void*, MultiMission*, int, bool& present) {
            present = true;
            return true;
        };
        start.mission_first_disc = [](void*, MultiMission*, int& disc) {
            disc = -1;
            return true;
        };
        start.force_disc = [](void*) { return true; };
        start.hide_cursor = [](void*) {
            if (WWMouseClass::Instance) WWMouseClass::Instance->HideCursor();
        };
        start.show_cursor = [](void*) {
            if (WWMouseClass::Instance) WWMouseClass::Instance->ShowCursor();
        };
        start.acquire_file = [](void*, const char* name, FileClass*& output) {
            auto file = std::make_unique<CCFileClass>(name);
            if (!file->Exists()) return false;
            output = file.release();
            return true;
        };
        start.release_file = [](void*, FileClass* file) { delete file; };
        start.play_movie = [](void*, int index, int theme, bool after, bool stretch, bool before) {
            if (index >= 0 && index < MovieInfo::Array.Count)
                Game::PlayMovie(MovieInfo::Array[index], theme, after, stretch, before);
        };
        start.stop_theme = [](void*, bool immediate) {
            if (native_audio_ready()) ThemeClass::Instance.Stop(immediate);
        };
        start.find_theme = [](void*, const char* name, int& index) {
            index = ThemeClass::Instance.FindIndex(name);
            return true;
        };
        start.play_theme = [](void*, int index) {
            if (native_audio_ready()) ThemeClass::Instance.Play(index);
        };
        start.queue_theme = [](void*, int index) {
            if (native_audio_ready()) ThemeClass::Instance.Queue(index);
        };
        start.load_world = [](void*, const char* name) { return ScenarioClass::ReadScenario(name); };
        start.dropship_dialog = [](void*) {
            throw std::runtime_error("StartingDropships requires the original dropship dialog");
        };
        // The host owns viewport sizing, options, menu assets and text input;
        // there is no original hidden surface or online lobby to retire.
        start.resize_display = [](void*, int, int) {};
        start.release_menu_assets = [](void*) {};
        start.apply_options = [](void*) {};
        start.disable_ime = [](void*) {};
        start.clear_online_state = [](void*) {};
        services.start = &start;
        world.context = &view;
        message_rect = DSurface::ViewBounds;
        world.message_rect = &message_rect;
        world.load_seed = [](void*, const char*) { return false; };
        world.generate_map = [](void*) { throw std::runtime_error("Random map generation is unavailable"); };
        world.legacy_starting_units = [](void*, bool official) { ScenarioClass::InitializeMultiplayer(official); };
        world.initialize_world_ini = [](void*, CCINIClass& ini, bool skip) {
            return ScenarioClass::InitializeWorldINI(&ini, skip);
        };
        world.legacy_finish_world = [](void*) { ScenarioClass::FinishWorld(); };
        world.set_shroud = [](void*, bool enabled) {
            if (!enabled) MapClass::Instance.RevealShroud(nullptr);
        };
        world.fade_loading_palette = [](void*) {};
        world.show_load_error = [](void*, const wchar_t*, const wchar_t*) {};
        world.configure_messages = [](void*, int x, int y, int width) {
            MessageListClass::Instance.Init(x, y, 6, 98, 14, -1, -1, 0, 20, 98, width);
        };
        world.finish_house = [](void*, HouseClass* house) { house->RecordStartingUnits(); };
        world.activate_sidebar = [](void*, int control) { SidebarClass::Instance.IsSidebarActive = control != 0; };
        services.world = &world;
        initialize.context = &view;
        initialize.set_map_size = [](void*, const RectangleStruct& rect, bool, bool, bool) {
            // This startup phase precedes map objects and packed terrain. The host
            // allocates fresh original Cells; the later map reader fills them.
            RadarClass::Instance.RadarClass::One_Time();
            if (!MapClass::Instance.CreateEmptyCells(rect, 0)) throw std::runtime_error("Invalid multiplayer map size");
        };
        initialize.prepare_mode = [](void*, MPGameModeClass* mode, bool official) {
            if (!mode->StartingPositionsToHouseBaseCells(official)) throw std::runtime_error("Invalid fixed starting positions");
        };
        initialize.start_mode = [](void*, MPGameModeClass* mode, bool official) {
            if (!mode->StartingPositionsToHouseBaseCells2(official)) throw std::runtime_error("Cannot assign starting positions");
        };
        initialize.start_special_mode = [](void*, bool official) {
            if (!SessionClass::Instance.MPGameMode->StartingPositionsToHouseBaseCells2(official))
                throw std::runtime_error("Cannot assign special-mode starting positions");
        };
        initialize.mode_ready = [](void*, MPGameModeClass* mode) { mode->UnfixAlliances(); };
        // The view owns its Tactical object; a new scenario reconstructs it.
        // The host sizes the viewport when it draws.
        initialize.tactical_rect = [](void*, TacticalClass*, const RectangleStruct&) {};
        // 0x005349C0 only mounts the theater archives and loads the palettes.
        // The one-time initialization below belongs to startup 0x00531680
        // (Map.One_Time at 0x00531708, then ObjectTypeClass::One_Time), which
        // runs once with the screen already sized. Native views run it per load,
        // after the generic archives are mounted, before any canvas exists.
        initialize.theater = [](void* p, int) {
            auto& v = *static_cast<MapViewHandle*>(p);
            if (!Theater::MountResourceMixes(v.scenario.Theater))
                throw std::runtime_error("Could not mount theater resources");
            ObjectTypeClass::One_Time();
            // Initialize the original hierarchy after archives and audio definitions,
            // before live cells are loaded. Allocation failures remain inside the
            // map-loading rollback boundary. The window is still empty here, so
            // DisplayClass::One_Time leaves the view unsized (0x72AD20 gives no
            // positive extent) until the host supplies the canvas.
            if (SessionClass::Instance.GameMode == GameMode::Campaign) RadarClass::Instance.RadarClass::One_Time();
            if (!FileSystem::LoadTheaterPalettes(v.scenario.Theater))
                throw std::runtime_error("Could not load theater palettes");
            if (v.world) load_world_palettes(*v.world);
            if (!v.world || !Theater::InitPaletteConverters(v.scenario.Theater, v.world->impl->presentation.unit_palette,
                                                            FileSystem::TEMPERAT_PAL))
                throw std::runtime_error("Could not initialize theater palette converters");
            Theater::LastTheater = v.scenario.Theater;
        };
        // 0x00534FA0 loads the side's sidebar archives; UiResources loads them
        // when the sidebar is first drawn.
        initialize.side = [](void*, int) { return true; };
        initialize.read_objects = [](void* p, ScenarioObjectReader, CCINIClass* ini) {
            auto& v = *static_cast<MapViewHandle*>(p);
            if (!ini || !DisplayClass::Instance.LoadTerrainFromINI(*ini))
                throw std::runtime_error(
                    "Could not load map terrain, theater catalog, packed cells or referenced TMP resources");
            if (!DisplayClass::Instance.LoadCellTagsFromINI(*ini))
                throw std::runtime_error("Could not load map cell tags");
            // Cell and overlay animations need their frame ranges before placement.
            // Loading art can append referenced types and replace Array.Items.
            if (v.world)
                for (int index = 0; index < AnimTypeClass::Array.Count; ++index) {
                    auto* type = AnimTypeClass::Array[index];
                    if (!type->Image) type->LoadFromINI(&v.world->art_ini);
                }
        };
        // Laser fences (0x452D40), fogged objects (0x5866C0) and the radar
        // redraw (0x657CE0) have no native counterpart; the host builds the
        // radar after loading.
        initialize.step = [](void*, ScenarioInitializationStep) {};
        services.initialize = &initialize;
        clear.context = &view;
        clear.stop_audio = [](void*) { stop_native_audio(); };
        // Native hosts list their scenarios themselves; no CampaignClass list.
        clear.init_campaigns = [](void*) {};
        services.clear = &clear;
    }
    ViewScenarioServices(const ViewScenarioServices&) = delete;
    ViewScenarioServices& operator=(const ViewScenarioServices&) = delete;
};
void set_error(MapViewHandle& view, const char* message) noexcept {
    std::snprintf(view.error, sizeof(view.error), "%s", message);
}
void release_world(MapViewHandle& view) noexcept {
    // Closing the asynchronous Options menu must finish its original resume
    // sequence before the audio owner and scenario are retired.
    if (view.options_paused && ScenarioClass::Instance == &view.scenario) {
        try {
            ScenarioClass::ResumeGame();
        } catch (...) {
        }
    }
    Game::SpecialDialog = 0;
    Game::PlayerWins = Game::PlayerLoses = false;
    Game::SidebarChanged = false;
    view.options_paused = false;
    reset_chrono_screen_effect();
    clear_sidebar_movies();
    stop_native_audio();
    SidebarClass::Instance.IsSidebarActive = false;
    // ScenarioClass::ClearWorld (0x006851F0) retires every scenario object and
    // type while cells and borrowed type art still live. It needs this view's
    // scenario services, which every map-view operation binds.
    try {
        ScenarioClass::ClearWorld();
    } catch (const std::exception& error) {
        runtime_log_event("map_clear_failed", "error=\"%.200s\"", error.what());
    } catch (...) {
        runtime_log_event("map_clear_failed", "");
    }
    view.world.reset();
    if (view.debug.original_speed) set_game_loop_speed(view.loop, view.debug.original_speed);
    view.debug = DebugState{};
    Game::ClearSidebarTabObject(nullptr);
    // Original world-object teardown ends with the beacon manager (0x53499B).
    // Empty/failed sessions also retire any remaining owned beacon slots.
    for (int i = 0; i < Game::PlanningEvents.Count; ++i)
        GameDelete(Game::PlanningEvents[i]);
    Game::PlanningEvents.Clear();
    Game::PlanningNodeAtAC4CCC = Game::PlanningNodeAtAC4C38 = Game::PlanningNodeAtAC4BF0 = nullptr;
    Game::PlanningInputBlocked = false;
    BeaconManagerClass::Instance.Reset();
    RadarEventClass::Clear();
    reset_mouse_input();
    // A map can close during a scripted input lock, before its unlock action.
    // Retire the session's global gate along with its Scenario state.
    Unsorted::UserInputLocked = false;
    view.scenario.UserInputLocked = false;
    Game::TypeSelectionIncludesMap = false;
    Game::TypeSelectionStarted = 0;
    Game::SetSelectionCommandMode(0);
    MessageListClass::Instance.Init(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    SidebarClass::Instance.SidebarClass::Init_Clear();
    GScreenClass::Buttons = nullptr;
    DisplayClass::TacticalButton.Zap();
    SidebarClass::BackgroundButton.Zap();
    RadarClass::RadarButton.Zap();
    RadarClass::DiplomacyButton.Zap();
    RadarClass::OptionsButton.Zap();
    SidebarClass::ToggleRepairButton.Zap();
    SidebarClass::ToggleRepairButton.SetShape(nullptr, 0, 0);
    SidebarClass::ToggleSellButton.Zap();
    SidebarClass::ToggleSellButton.SetShape(nullptr, 0, 0);
    for (auto& tab : SidebarClass::TabButtons) {
        tab.Zap();
        tab.SetShape(nullptr, 0, 0);
    }
    for (int i = 0; i < 240; ++i)
        SelectClass::Array()[i].Zap();
    for (auto& button : TabClass::CommandButtons) {
        button.Zap();
        button.SetShape(nullptr, 0, 0);
    }
    for (auto* button : {&TabClass::CollapseButton, &TabClass::ExpandButton}) {
        button->Zap();
        button->SetShape(nullptr, 0, 0);
    }
    TabClass::Instance.ThumbActive = true;
    view.tooltips.SetState(false);
    while (view.tooltips.ToolTips.Count)
        view.tooltips.Remove(view.tooltips.ToolTips[0]->GadgetID);
    view.tooltip_platform.timer_owner = nullptr;
    view.keyboard.Reset();
    view.pointer_inside = false;
    const int speed_level = view.loop.speed_level;
    const bool host_paused = view.loop.host_paused;
    view.loop = GameLoopState{};
    view.loop.speed_level = speed_level;
    view.loop.host_paused = host_paused;
    view.ui_resources.reset();
    view.ui_render = {};
    view.shapes_prepared = false;
    MixFileClass::UnloadSidebarMixes();
    RadarClass::Instance.ReleaseTerrainRadar();
    MapClass::Instance.ReleaseCellStorage();
    IsometricTileTypeClass::ClearTileSetCatalog();
    IsometricTileTypeClass::ClearSlopeDepthShapes();
    Theater::UnmountResourceMixes();
    view.terrain_loaded = false;
    TacticalClass::ViewBounds = {};
    DSurface::ViewBounds = DSurface::SidebarBounds = DSurface::WindowBounds = {};
}
// Same replacement ClearWorld performs (0x006853B4); repeated here so a clear that
// failed before reaching it still leaves a fresh object for the next load.
void renew_tactical() {
    delete TacticalClass::Instance;
    TacticalClass::Instance = new TacticalClass();
}
void reset_world(MapViewHandle& view) {
    release_world(view);
    // Only retire frontend records owned by this view; external native callers
    // may supply their own Session/NodeName records across reloads.
    if (SessionClass::Instance.MPGameMode == view.battle_mode.get()) SessionClass::Instance.MPGameMode = nullptr;
    view.battle_mode.reset();
    const int player = NodeNameType::Array.FindItemIndex(&view.local_player);
    if (player >= 0) NodeNameType::Array.RemoveItem(player);
    set_game_loop_speed(view.loop, view.loop.speed_level);
    if (view.rules) {
        view.rules = std::make_unique<RulesClass>();
        RulesClass::Instance = view.rules.get();
    }
    Unsorted::CurrentFrame = 0;
    view.scenario.Reset();
    view.scenario.Random = Randomizer(0);
    renew_tactical();
}
}
MapViewHandle::MapViewHandle(ResourceHandle& source)
    : resources(&source), rules(RulesClass::Instance ? nullptr : std::make_unique<RulesClass>()) {}
MapViewHandle::~MapViewHandle() {
    if (SessionClass::Instance.MPGameMode == battle_mode.get()) SessionClass::Instance.MPGameMode = nullptr;
    const int player = NodeNameType::Array.FindItemIndex(&local_player);
    if (player >= 0) NodeNameType::Array.RemoveItem(player);
    delete TacticalClass::Instance;
}
bool create_map_view(ResourceHandle& resources, MapViewHandle*& output) noexcept {
    if (output || active || ScenarioClass::Instance || TacticalClass::Instance || MapClass::Instance.Cells.Items ||
        IsometricTileTypeClass::Array.Count)
        return false;
    try {
        // Reset the original session before Scenario construction: Reset reads
        // its mode, so a previous skirmish must not leak into a new view.
        SessionClass::Instance.~SessionClass();
        ::new (&SessionClass::Instance) SessionClass{};
        auto view = std::make_unique<MapViewHandle>(resources);
        // Bind the original objects once for this view's lifetime. Startup
        // (0x0052C694) creates the first Tactical object before any scenario.
        TacticalClass::Instance = new TacticalClass();
        ScenarioClass::Instance = &view->scenario;
        InputManagerClass::Instance = &view->keyboard;
        if (view->rules) RulesClass::Instance = view->rules.get();
        Game::IsFocused = true;
        output = view.release();
        active = output;
        return true;
    } catch (...) {
        return false;
    }
}
bool with_map_view(MapViewHandle& view, void (*operation)(void*), void* argument) noexcept {
    if (active != &view || !operation) return false;
    if (ScenarioClass::Instance && ScenarioClass::Instance != &view.scenario) {
        set_error(view, "Another Scenario owns the active world");
        return false;
    }
    ClockReadScope clock_scope;
    view.tooltip_platform.pointer = &view.pointer;
    view.tooltip_platform.pointer_visible = &view.pointer_inside;
    ToolTipScope tooltip_scope(view.tooltip_platform, view.tooltips);
    view.tooltips.SetState(GameOptionsClass::Instance.Tooltips);
    // Zero-delay mouse messages and headless timer delivery also measure text.
    // The resource session owns GAME.FNT; its original global is only borrowed.
    auto* previous_font = BitFont::Instance;
    if (view.ui_resources && view.ui_resources->font()) BitFont::Instance = view.ui_resources->font();
    struct RestoreFont {
        BitFont* old;
        ~RestoreFont() { BitFont::Instance = old; }
    } restore_font{previous_font};
    ++view.operation_depth;
    struct EndOperation {
        MapViewHandle& view;
        ~EndOperation() { --view.operation_depth; }
    } end{view};
    struct Context {
        MapViewHandle& view;
        void (*operation)(void*);
        void* argument;
        bool completed = false, com_failed = false;
        std::int32_t com_code = 0;
    } context{view, operation, argument};
    try {
        std::string error;
        const bool result = with_resources(
            *view.resources,
            [](void* pointer) {
                auto& context = *static_cast<Context*>(pointer);
                auto services = default_scenario_runtime();
                ViewScenarioServices bound(context.view, services);
                auto* volume = Audio::MasterVolume ? reinterpret_cast<VolumeStruct*>(Audio::MasterVolume)
                                                   : &context.view.detached_pause_volume;
                ToolTipManager* tooltip = &context.view.tooltips;
                ScenarioPauseServices pause{};
                pause.context = &context.view;
                pause.volume = &volume;
                pause.tooltip = &tooltip;
                pause.tooltips_enabled = &GameOptionsClass::Instance.Tooltips;
                pause.suspend_audio = [](void*) { Audio::SuspendAll(); };
                pause.resume_audio = [](void*) { Audio::ResumeAll(); };
                pause.set_input_paused = [](void* p, bool) { static_cast<MapViewHandle*>(p)->keyboard.Reset(); };
                pause.set_tooltip_state = [](void*, ToolTipManager* t, bool enabled) { t->SetState(enabled); };
                pause.release_mouse_capture = [](void*) { reset_mouse_input(); };
                pause.set_cursor = [](void*, int index, bool minimap) {
                    MouseClass::Instance.SetCursor(static_cast<MouseCursorType>(index), minimap);
                };
                pause.restore_cursor = [](void*) { MouseClass::Instance.RestoreCursor(); };
                pause.hide_cursor = [](void*) {
                    if (WWMouseClass::Instance) WWMouseClass::Instance->HideCursor();
                };
                pause.show_cursor = [](void*) {
                    if (WWMouseClass::Instance) WWMouseClass::Instance->ShowCursor();
                };
                // Native presentation is asynchronous; invalidate the frame already
                // handed to the host rather than entering a nested device loop.
                pause.render_frame = [](void* p) { ++static_cast<MapViewHandle*>(p)->ui_generation; };
                services.pause = &pause;
                services.context = &context.view;
                services.variable_changed = [](void*, bool global, int index) {
                    if (global)
                        TagClass::NotifyGlobalChanged(index);
                    else
                        TagClass::NotifyLocalChanged(index);
                };
                context.completed = with_scenario_runtime(
                    services,
                    [](void* p) {
                        auto& c = *static_cast<Context*>(p);
                        // Preserve the COM failure before the nested noexcept scopes
                        // reduce callback failures to a boolean status.
                        if (!with_map_world(
                                c.view.world.get(),
                                [](void* pointer) {
                                    auto& c = *static_cast<Context*>(pointer);
                                    try {
                                        c.operation(c.argument);
                                    } catch (const ComError& error) {
                                        c.com_failed = true;
                                        c.com_code = error.Error();
                                    }
                                },
                                &c))
                            throw std::runtime_error("Native map-world operation failed");
                    },
                    &context);
            },
            &context, error);
        if (context.com_failed) {
            char message[64];
            std::snprintf(message, sizeof(message), "Original COM failure: 0x%08X",
                          static_cast<unsigned>(context.com_code));
            set_error(view, message);
            return false;
        }
        if (!result || !context.completed) {
            set_error(view, error.empty() ? "Map operation unavailable" : error.c_str());
            return false;
        }
        // Successful binding does not imply that the callback succeeded.
        // Operation owners clear their own error before starting new work.
        return true;
    } catch (const std::exception& error) {
        set_error(view, error.what());
        return false;
    } catch (...) {
        set_error(view, "Map operation failed");
        return false;
    }
}
bool initialize_empty_map_view(MapViewHandle& view, const RectangleStruct& bounds, char level) noexcept {
    if (active != &view) return false;
    if (view.operation_depth) {
        set_error(view, "Cannot reload a map during an active operation");
        return false;
    }
    view.state = MapViewState::empty;
    ++view.generation;
    struct Context {
        MapViewHandle& view;
        RectangleStruct bounds;
        char level;
        bool complete = false;
    } context{view, bounds, level};
    const bool scoped = with_map_view(
        view,
        [](void* pointer) {
            auto& context = *static_cast<Context*>(pointer);
            reset_world(context.view);
            MapClass::Instance.MaxLevel = 13;
            context.complete = MapClass::Instance.CreateEmptyCells(context.bounds, context.level);
        },
        &context);
    if (!scoped || !context.complete) {
        MapClass::Instance.ReleaseCellStorage();
        if (scoped) set_error(view, "Could not allocate the requested map dimensions or level");
        view.state = MapViewState::failed;
        return false;
    }
    view.state = MapViewState::ready;
    return true;
}
static bool valid_skirmish(const SkirmishOptions& setup) noexcept {
    if (setup.players[0].kind != 3 || setup.money < 0 || setup.money > 1000000 ||
        setup.unit_count < 0 || setup.unit_count > 100 || setup.game_speed < 0 || setup.game_speed > 6) return false;
    unsigned colors = 0, starts = 0;
    int count = 0;
    bool opponent = false;
    for (int i = 0; i < 8; ++i) {
        const auto& player = setup.players[i];
        if (i && (player.kind < -1 || player.kind > 2)) return false;
        if (player.kind == -1) continue;
        ++count;
        if ((player.country != -2 && (player.country < 0 || player.country > 255)) ||
            (player.color != -2 && (player.color < 0 || player.color > 7)) ||
            (player.start != -2 && (player.start < 0 || player.start > 7)) ||
            player.team < -1 || player.team > 3) return false;
        if (player.color >= 0) {
            if (colors & (1u << player.color)) return false;
            colors |= 1u << player.color;
        }
        if (player.start >= 0) {
            if (starts & (1u << player.start)) return false;
            starts |= 1u << player.start;
        }
        if (i && (setup.players[0].team == -1 || player.team != setup.players[0].team)) opponent = true;
    }
    return count >= 2 && opponent;
}
static bool load_map_request(MapViewHandle& view, const char* filename, std::uint32_t length,
                             const SkirmishOptions* skirmish) noexcept {
    if (active != &view) return false;
    if (view.operation_depth) {
        set_error(view, "Cannot reload a map during an active operation");
        return false;
    }
    if (!filename || !length || length >= sizeof(view.scenario.FileName) || std::memchr(filename, 0, length)) {
        set_error(view, "Invalid map filename");
        return false;
    }
    // Capture before teardown: the caller may borrow the current Scenario name.
    char name[260]{};
    std::memcpy(name, filename, length);
    view.error[0] = 0;
    if (!view.continuation_loading) {
        EvadeClass::Instance.Reset();
        view.transition = {};
        view.transition_prepared = false;
        view.next_campaign = -1;
    }
    view.state = MapViewState::empty;
    ++view.generation;
    struct Context {
        MapViewHandle& view;
        const char* filename;
        const char* failure = "Map load failed";
        bool complete = false;
        char detail[512]{};
        const SkirmishOptions* skirmish = nullptr;
    } context{view, name, "Map load failed", false, {}, skirmish};
    const bool scoped = with_map_view(
        view,
        [](void* pointer) {
            auto& c = *static_cast<Context*>(pointer);
            reset_world(c.view);
            runtime_crash_checkpoint(RuntimePhase::map_load, Unsorted::CurrentFrame);
            runtime_log_map(c.filename, -1, int(c.view.scenario.Difficulty1), "", 0);
            runtime_log_event("map_load_begin", "");
            struct Rollback {
                Context& c;
                ~Rollback() {
                    if (!c.complete) release_world(c.view);
                }
            } rollback{c};
            c.failure = "Could not mount map packages";
            if (!MixFileClass::LoadMapMixes()) return;
            c.failure = "Could not load original KEYBOARDMD.INI command bindings";
            if (!initialize_player_commands()) return;
            c.view.world = std::make_unique<MapWorld>(c.view);
            auto& ini = c.view.world->map_ini;
            c.failure = "Map file not found in the resource directory or mounted MIX packages";
            CCFileClass file(c.filename);
            if (!file.Exists()) return;
            for (const char* resource :
                 {c.filename, "RULESMD.INI", "ARTMD.INI", "AIMD.INI", "KEYBOARDMD.INI", "RA2MD.INI"})
                runtime_log_resource(resource);
            c.failure = "Could not read map INI";
            if (ini.ReadCCFile(&file, false, false) <= 0) return;
            c.failure = "Map INI is missing the [Map] section";
            if (!ini.GetSection("Map")) return;
            SessionClass::Instance.GameMode =
                ini.ReadBool("Basic", "MultiplayerOnly", false) ? GameMode::Skirmish : GameMode::Campaign;
            if (c.skirmish && SessionClass::Instance.GameMode != GameMode::Skirmish) {
                c.failure = "Selected map is not a multiplayer map";
                return;
            }
            if (SessionClass::IsCampaign() && c.view.campaign_difficulty >= 0)
                GameOptionsClass::Instance.Difficulty = c.view.campaign_difficulty;
            auto& scenario = c.view.scenario;
            std::strcpy(scenario.FileName, c.filename);
            c.failure = "Could not load map terrain, theater catalog, packed cells or referenced TMP resources";
            GameOptionsClass::Instance.DetailLevel = 2;
            CCFileClass options_file("RA2MD.INI");
            CCINIClass options_ini;
            if (options_file.Exists() && options_ini.ReadCCFile(&options_file, false, false) > 0) {
                auto& options = GameOptionsClass::Instance;
                options.DetailLevel = std::clamp(options_ini.ReadInteger("Options", "DetailLevel", 2), 0, 2);
                options.ScrollRate = std::clamp(options_ini.ReadInteger("Options", "ScrollRate", 3), 0, 7);
                options.ScrollMethod = std::clamp(options_ini.ReadInteger("Options", "ScrollMethod", 0), 0, 2);
                options.AutoScroll = options_ini.ReadBool("Options", "AutoScroll", true);
                options.Tooltips = options_ini.ReadBool("Options", "ToolTips", true);
            }
            struct Load {
                MapWorld& world;
                char* detail;
                std::size_t size;
                bool ok = false;
                const SkirmishOptions* skirmish;
            } load{*c.view.world, c.detail, sizeof(c.detail), false, c.skirmish};
            if (!with_map_world(
                    c.view.world.get(),
                    [](void* p) {
                        auto& l = *static_cast<Load*>(p);
                        l.ok = load_map_world(l.world, l.detail, l.size, l.skirmish);
                    },
                    &load) ||
                !load.ok)
                return;
            if (c.view.continuation_loading) {
                if (!HouseClass::CurrentPlayer) {
                    c.failure = "The next scenario has no player house";
                    return;
                }
                EvadeClass::Instance.Do();
            }
            // A narrow/empty radar fit or allocation failure must not discard an
            // otherwise usable map. Its absence is exposed by the radar query API.
            RadarClass::Instance.BuildTerrainRadar();
            if (HouseClass::CurrentPlayer) {
                // Opening credits are initial state, not animated income from the previous map.
                TabClass::Instance.TabData.Update(true);
            }
            if (native_audio_ready()) {
                VoxClass::EVAIndex = HouseClass::CurrentPlayer && HouseClass::CurrentPlayer->Type
                                         ? HouseClass::CurrentPlayer->Type->SideIndex
                                         : 0;
            }
            prepare_background_invalidation_extent();
            // Sight acquired while the map loaded is settled before the first frame.
            visibility_frame_end();
            c.view.terrain_loaded = true;
            c.complete = true;
        },
        &context);
    if (!scoped || !context.complete) {
        if (context.detail[0])
            set_error(view, context.detail);
        else if (scoped)
            set_error(view, context.failure);
        view.state = MapViewState::failed;
        runtime_log_event("map_load_failed", "error=\"%.480s\"", view.error);
        return false;
    }
    view.state = MapViewState::ready;
    std::memcpy(view.loaded_filename, name, sizeof(name));
    if (!view.continuation_loading) std::memcpy(view.transition.filename, name, sizeof(name));
    const auto* player = HouseClass::CurrentPlayer;
    runtime_log_map(name, int(SessionClass::Instance.GameMode), int(view.scenario.Difficulty1),
                    player && player->Type ? player->Type->ID : "", view.world->impl->world_id);
    runtime_log_rng(view.scenario.Random, Randomizer::Global);
    runtime_log_event("map_ready", "speed_level=%d", view.loop.speed_level);
    return true;
}
bool load_map_view(MapViewHandle& view, const char* filename, std::uint32_t length) noexcept {
    return load_map_request(view, filename, length, nullptr);
}
bool set_campaign_difficulty(MapViewHandle& view, int difficulty) noexcept {
    if (active != &view || view.operation_depth || view.state != MapViewState::empty ||
        difficulty < 0 || difficulty > 2) return false;
    view.campaign_difficulty = difficulty;
    return true;
}
bool load_skirmish_view(MapViewHandle& view, const char* filename, std::uint32_t length,
                        const SkirmishOptions& options) noexcept {
    if (!valid_skirmish(options)) {
        set_error(view, "Invalid skirmish players or options");
        return false;
    }
    const auto copy = options;
    return load_map_request(view, filename, length, &copy);
}

bool get_mission_transition(const MapViewHandle& view, MissionTransitionInfo& output) noexcept {
    if (active != &view) return false;
    output = view.transition;
    return true;
}
bool begin_mission_transition(MapViewHandle& view, std::uint64_t id) noexcept {
    if (active != &view || view.operation_depth || !id || view.transition.id != id) return false;
    auto& transition = view.transition;
    if (transition.state != MissionTransitionState::next && transition.state != MissionTransitionState::retry &&
        transition.state != MissionTransitionState::failed)
        return false;
    if (!view.transition_prepared) {
        const bool prepared = with_map_view(view, [](void* p) {
            auto& v = *static_cast<MapViewHandle*>(p);
            if (v.transition.result == GameResult::victory) {
                v.scenario.Stage = v.next_stage;
                EvadeClass::Instance.Save();
                v.scenario.TechLevel =
                    std::bit_cast<int>(static_cast<unsigned>(v.scenario.TechLevel) + 1u);
            }
            // A defeat reuses the entry handoff; it never saves the defeated world.
            v.transition_prepared = true;
        }, &view);
        if (!prepared) return false;
    }
    transition.state = MissionTransitionState::loading;
    transition.error[0] = 0;
    return true;
}
bool load_mission_transition(MapViewHandle& view) noexcept {
    if (active != &view || view.operation_depth || view.transition.state != MissionTransitionState::loading ||
        !view.transition_prepared)
        return false;
    view.continuation_loading = true;
    const bool ok = load_map_view(view, view.transition.filename,
                                 static_cast<std::uint32_t>(std::strlen(view.transition.filename)));
    view.continuation_loading = false;
    if (ok) {
        view.transition = {};
        std::memcpy(view.transition.filename, view.loaded_filename, sizeof(view.loaded_filename));
        view.transition_prepared = false;
    } else {
        view.transition.state = MissionTransitionState::failed;
        std::snprintf(view.transition.error, sizeof(view.transition.error), "%.511s", view.error);
    }
    return ok;
}
void fail_mission_transition(MapViewHandle& view, const char* message) noexcept {
    if (active != &view || view.operation_depth || view.transition.state != MissionTransitionState::loading) return;
    view.transition.state = MissionTransitionState::failed;
    std::snprintf(view.transition.error, sizeof(view.transition.error), "%s", message ? message : "Mission loader failed");
    set_error(view, view.transition.error);
}
void destroy_map_view(MapViewHandle*& view) noexcept {
    if (!view) return;
    if (view != active) return;
    if (view->operation_depth) {
        set_error(*view, "Cannot close a map during an active operation");
        return;
    }
    runtime_crash_checkpoint(RuntimePhase::cleanup, Unsorted::CurrentFrame);
    runtime_log_event("map_close_begin", "map=\"%.240s\"", view->scenario.FileName);
    // Stop the world's consumers before calling this boundary. Cells release
    // their borrowed resources while the resource environment is still alive.
    if (!with_map_view(*view, [](void* pointer) { release_world(*static_cast<MapViewHandle*>(pointer)); }, view))
        release_world(*view);
    const bool owned_rules = view->rules != nullptr;
    active = nullptr;
    delete view;
    view = nullptr;
    ScenarioClass::Instance = nullptr;
    InputManagerClass::Instance = nullptr;
    if (owned_rules) RulesClass::Instance = nullptr;
    runtime_log_event("map_close_complete", "");
}
bool get_map_view_info(const MapViewHandle& view, MapViewInfo& output) noexcept {
    if (active != &view) return false;
    const auto& map = MapClass::Instance;
    // A load that failed between the delete and the new of ClearWorld leaves no object.
    const Point2D camera = TacticalClass::Instance ? TacticalClass::Instance->TacticalPos : Point2D{};
    output = {view.state,
              view.generation,
              map.MapRect.Width,
              map.MapRect.Height,
              map.Cells.Capacity,
              view.terrain_loaded,
              static_cast<int>(view.scenario.Theater),
              map.VisibleRect.X,
              map.VisibleRect.Y,
              map.VisibleRect.Width,
              map.VisibleRect.Height,
              TacticalClass::ViewBounds.Width,
              TacticalClass::ViewBounds.Height,
              camera.X,
              camera.Y,
              view.camera_generation};
    if (view.world) {
        output.presentation_revision = view.world->presentation_revision;
        output.resource_revision = view.world->resource_revision;
        output.simulation_tick = view.world->simulation_tick;
    }
    return true;
}
bool set_map_viewport(MapViewHandle& view, int width, int height) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded || width < 1 || height < 1 || width > 8192 ||
        height > 8192)
        return false;
    if (TacticalClass::ViewBounds.Width == width && TacticalClass::ViewBounds.Height == height &&
        DSurface::ViewBounds.Width == width && DSurface::ViewBounds.Height == height)
        return true;
    Point2D center;
    if (!TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height) {
        auto& scenario = view.scenario;
        CellStruct home{static_cast<short>((scenario.Width + scenario.Height) / 2),
                        static_cast<short>((scenario.Width + scenario.Height) / 2)};
        // FinishWorld already selected HomeCell or AltHomeCell into View1.
        if (scenario.View1 != CellStruct{0, 0}) home = scenario.View1;
        const auto* cell = MapClass::Instance.TryGetCellAt(home);
        center = TacticalClass::CoordsToScreen(CoordStruct{int(home.X) * 256 + 128, int(home.Y) * 256 + 128, 0});
        center.Y -= cell ? 15 * int(static_cast<signed char>(cell->Level)) : 0;
    } else {
        center = {TacticalClass::Instance->TacticalPos.X + TacticalClass::ViewBounds.Width / 2,
                  TacticalClass::Instance->TacticalPos.Y + TacticalClass::ViewBounds.Height / 2};
    }
    TacticalClass::ViewBounds = {0, 0, width, height};
    // Map-only hosts still use the original Display input surface. Publish
    // its dimensions and tactical gadget before accepting pointer gestures.
    if (!with_map_view(
            view, [](void*) { DisplayClass::Instance.Set_View_Dimensions(TacticalClass::ViewBounds); }, nullptr))
        return false;
    return center_map_view(view, center.X, center.Y);
}
bool set_game_view_size(MapViewHandle& view, int width, int height) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded || width < 640 || height < 480 ||
        width > 8192 || height > 8192) {
        char detail[256];
        std::snprintf(
            detail, sizeof(detail),
            "width=%d height=%d expected_width=[640,8192] expected_height=[480,8192] active=%d depth=%u terrain=%d",
            width, height, active == &view, view.operation_depth, view.terrain_loaded);
        record_drawing_diagnostic(DrawingStatus::invalid_argument, "view_size_setup", "invalid_size_or_view_state",
                                  detail, __FILE__, __LINE__);
        set_error(view, drawing_failure());
        return false;
    }
    struct Resize {
        MapViewHandle& view;
        int width, height;
    } resize{view, width, height};
    return with_map_view(
        view,
        [](void* pointer) {
            const auto& r = *static_cast<Resize*>(pointer);
            auto& view = r.view;
            const auto before = TacticalClass::Instance->TacticalPos;
            if (DSurface::WindowBounds.Width != r.width || DSurface::WindowBounds.Height != r.height) {
                invalidate_background_all();
                runtime_log_event("canvas_resize", "width=%d height=%d camera=%d,%d", r.width, r.height, before.X,
                                  before.Y);
            }
            if (!TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height) {
                auto& scenario = view.scenario;
                CellStruct home{static_cast<short>((scenario.Width + scenario.Height) / 2),
                                static_cast<short>((scenario.Width + scenario.Height) / 2)};
                if (scenario.View1 != CellStruct{0, 0}) home = scenario.View1;
                auto center = TacticalClass::CoordsToScreen({int(home.X) * 256 + 128, int(home.Y) * 256 + 128, 0});
                if (const auto* cell = MapClass::Instance.TryGetCellAt(home))
                    center.Y -= 15 * int(static_cast<signed char>(cell->Level));
                TacticalClass::Instance->TacticalCoord1 = center;
            }
            DSurface::WindowBounds = {0, 0, r.width, r.height};
            DisplayClass::Instance.Set_View_Dimensions({0, 0, r.width - 168, r.height - 32});
            // Original scenario/display startup calls Tab.Activate(1) at
            // 0x0067E69F / 0x00561113; Sidebar.Activate writes this flag at
            // 0x006A7DA6. Native layout and gadget registration are separate,
            // but the visible sidebar must publish the same activation state.
            SidebarClass::Instance.IsSidebarActive = true;
            if (before != TacticalClass::Instance->TacticalPos) ++view.camera_generation;
        },
        &resize);
}
bool get_game_view_layout(MapViewHandle& view, GameViewLayout& output) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded || DSurface::WindowBounds.Width < 640)
        return false;
    GameViewLayout result;
    struct Query {
        MapViewHandle& view;
        GameViewLayout& result;
    } query{view, result};
    if (!with_map_view(
            view,
            [](void* pointer) {
                auto& q = *static_cast<Query*>(pointer);
                auto& sidebar = SidebarClass::Instance;
                q.result = {DSurface::WindowBounds,
                            DSurface::ViewBounds,
                            DSurface::SidebarBounds,
                            RadarClass::Instance.GetPanelBounds(),
                            TabClass::Instance.GetCommandBarBounds(),
                            sidebar.CameoPosition,
                            sidebar.CameoPitch,
                            sidebar.ScrollPosition,
                            q.view.scenario.PlayerSideIndex,
                            sidebar.GetUsableCameoCount(),
                            sidebar.GetVisibleCameoCount()};
            },
            &query))
        return false;
    output = result;
    return true;
}
bool center_map_view(MapViewHandle& view, int x, int y) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width ||
        !TacticalClass::ViewBounds.Height)
        return false;
    const auto before = TacticalClass::Instance->TacticalPos;
    if (!TacticalClass::Instance->FocusView({x, y})) return false;
    if (before != TacticalClass::Instance->TacticalPos) ++view.camera_generation;
    return true;
}
bool scroll_map_view(MapViewHandle& view, int dx, int dy) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width ||
        !TacticalClass::ViewBounds.Height)
        return false;
    const auto& center = TacticalClass::Instance->TacticalCoord1;
    const auto bounded = [](std::int64_t value) {
        return static_cast<int>(std::clamp<std::int64_t>(value, INT_MIN, INT_MAX));
    };
    return center_map_view(view, bounded(std::int64_t(center.X) + dx), bounded(std::int64_t(center.Y) + dy));
}
bool get_map_radar_info(MapViewHandle& view, MapRadarInfo& output) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded) return false;
    auto& radar = RadarClass::Instance;
    if (!radar.unknown_123C) return false;
    const auto& rect = radar.unknown_rect_149C;
    CellStruct center;
    const bool valid =
        TacticalClass::ViewBounds.Width > 0 && TacticalClass::ViewBounds.Height > 0 &&
        TacticalClass::Instance->PickTerrainCell(
            {TacticalClass::ViewBounds.Width / 2, TacticalClass::ViewBounds.Height / 2}, TacticalClass::ViewBounds,
            center) &&
        radar.UpdateViewportFrame(center, {TacticalClass::ViewBounds.Width, TacticalClass::ViewBounds.Height});
    const auto frame = valid ? radar.unknown_rect_14DC : RectangleStruct{};
    // The host minimap API remains panel-local; original Radar fields are
    // sidebar-local. Convert only here, at the host boundary.
    const int x = int(radar.unknown_11F0), y = int(radar.unknown_11F4);
    output = {140,
              108,
              rect.X - x,
              rect.Y - y,
              rect.Width,
              rect.Height,
              int(radar.unknown_1240),
              int(radar.unknown_1244),
              valid ? frame.X - x : 0,
              valid ? frame.Y - y : 0,
              frame.Width,
              frame.Height,
              valid};
    return true;
}
bool copy_map_radar_pixels(MapViewHandle& view, std::uint16_t* output, std::uint32_t capacity) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded) return false;
    const auto& radar = RadarClass::Instance;
    return radar.CopyTerrainRadar(output, capacity);
}
bool center_map_from_radar(MapViewHandle& view, int x, int y) noexcept {
    if (active != &view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width ||
        !TacticalClass::ViewBounds.Height)
        return false;
    struct Click {
        MapViewHandle& view;
        int x, y;
        bool moved = false;
    } click{view, x, y};
    // The host passes panel-local pixels; original Radar fields are sidebar-local.
    // The click itself is the Action body of 0x006539D0 (RadarClass::Navigate).
    return with_map_view(
               view,
               [](void* pointer) {
                   auto& c = *static_cast<Click*>(pointer);
                   auto& radar = RadarClass::Instance;
                   const auto before = TacticalClass::Instance->TacticalPos;
                   c.moved = radar.Navigate({c.x + int(radar.unknown_11F0), c.y + int(radar.unknown_11F4)});
                   if (before != TacticalClass::Instance->TacticalPos) ++c.view.camera_generation;
               },
               &click) &&
           click.moved;
}
DrawingStatus draw_map_view(MapViewHandle& view, const MapDrawingContext& drawing, MapDrawStatistics& output) noexcept {
    output = {};
    if (!valid_drawing_context(drawing.types))
        return DrawingStatus::unsupported;
    clear_drawing_failure();
    view.error[0] = 0;
    if (active != &view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width ||
        !TacticalClass::ViewBounds.Height)
        return DrawingStatus::unavailable;
    struct Frame {
        MapViewHandle& view;
        const MapDrawingContext& drawing;
        MapDrawStatistics& stats;
        DrawingStatus result = DrawingStatus::unavailable;
    } frame{view, drawing, output};
    if (!with_map_view(
            view,
            [](void* pointer) {
                auto& frame = *static_cast<Frame*>(pointer);
                frame.result = draw_tactical_view(*TacticalClass::Instance, frame.view.world.get(), frame.drawing,
                                                  TacticalClass::ViewBounds, frame.stats);
            },
            &frame)) {
        if (drawing_failure()[0]) set_error(view, drawing_failure());
        return DrawingStatus::backend_failure;
    }
    if (frame.result != DrawingStatus::drawn && frame.result != DrawingStatus::skipped)
        set_error(view, drawing_failure()[0] ? drawing_failure() : drawing_status_name(frame.result));
    return frame.result;
}
namespace {
// The drawing of draw_game_view for a caller that already holds the view binding.
// The operation is the original render phase; it runs inside the drawing target.
DrawingStatus draw_game_view_bound(MapViewHandle& view, const MapDrawingContext& drawing, MapDrawStatistics& output,
                                   void (*operation)()) {
    output = {};
    if (!view.terrain_loaded || DSurface::WindowBounds.Width < 640) return DrawingStatus::unavailable;
    if (!view.ui_resources) view.ui_resources = std::make_unique<UiResources>();
    auto& resources = *view.ui_resources;
    if (!resources.load(view.scenario.PlayerSideIndex)) {
        set_error(view, resources.error());
        return DrawingStatus::backend_failure;
    }
    GameUiFrame context{drawing, resources, output};
    context.advance_presentation = current_game_loop() != nullptr;
    context.render_state = &view.ui_render;
    const auto result = with_game_ui_frame(context, operation);
    if (result != DrawingStatus::drawn && result != DrawingStatus::skipped)
        set_error(view, drawing_failure()[0] ? drawing_failure() : drawing_status_name(result));
    return result;
}
}
DrawingStatus draw_game_view(MapViewHandle& view, const MapDrawingContext& drawing,
                             MapDrawStatistics& output) noexcept {
    output = {};
    if (!valid_drawing_context(drawing.types))
        return DrawingStatus::unsupported;
    clear_drawing_failure();
    view.error[0] = 0;
    if (active != &view || view.operation_depth || !view.terrain_loaded || DSurface::WindowBounds.Width < 640)
        return DrawingStatus::unavailable;
    struct Frame {
        MapViewHandle& view;
        const MapDrawingContext& drawing;
        MapDrawStatistics& statistics;
        DrawingStatus result = DrawingStatus::unavailable;
    } frame{view, drawing, output};
    if (!with_map_view(
            view,
            [](void* pointer) {
                auto& f = *static_cast<Frame*>(pointer);
                f.result = draw_game_view_bound(f.view, f.drawing, f.statistics, [] { present_game_screen(); });
            },
            &frame)) {
        if (drawing_failure()[0]) set_error(view, drawing_failure());
        return DrawingStatus::backend_failure;
    }
    return frame.result;
}
bool submit_game_input(MapViewHandle& view, const GameInputEvent& event, GameInputResult& result) noexcept {
    result = {};
    if (active != &view || view.operation_depth || !view.terrain_loaded ||
        static_cast<unsigned>(event.kind) > static_cast<unsigned>(GameInputKind::focus_gained) || event.x < -65536 ||
        event.x > 65536 || event.y < -65536 || event.y > 65536 || event.code > 255)
        return false;
    struct Input {
        MapViewHandle& view;
        const GameInputEvent& event;
        GameInputResult& result;
    } input{view, event, result};
    if (event.kind == GameInputKind::focus_gained) set_game_view_activity(view, true, view.loop.host_paused);
    if (event.kind == GameInputKind::focus_lost) set_game_view_activity(view, false, view.loop.host_paused);
    return with_map_view(
        view,
        [](void* pointer) {
            auto& i = *static_cast<Input*>(pointer);
            const auto before = TacticalClass::Instance->TacticalPos;
            runtime_crash_checkpoint(RuntimePhase::input, Unsorted::CurrentFrame);
            runtime_log_event(
                "input", "kind=%u code=%u pressed=%d modifiers=%u pos=%d,%d wheel=%d camera=%d,%d canvas=%d,%d",
                unsigned(i.event.kind), i.event.code, i.event.pressed, i.event.modifiers, i.event.x, i.event.y,
                i.event.wheel, before.X, before.Y, DSurface::WindowBounds.Width, DSurface::WindowBounds.Height);
            if (i.event.kind == GameInputKind::focus_gained) {
                runtime_log_event("input_result", "kind=%u consumed=0 warp=0 pointer=0,0 camera=%d,%d",
                                  unsigned(i.event.kind), before.X, before.Y);
                return;
            }
            if (i.event.kind == GameInputKind::focus_lost || i.event.kind == GameInputKind::pointer_leave)
                i.view.pointer_inside = false;
            else if (i.event.kind != GameInputKind::key) {
                i.view.pointer = {i.event.x, i.event.y};
                const auto& canvas =
                    DSurface::WindowBounds.Width > 0 ? DSurface::WindowBounds : TacticalClass::ViewBounds;
                i.view.pointer_inside =
                    i.event.x >= 0 && i.event.y >= 0 && i.event.x < canvas.Width && i.event.y < canvas.Height;
            }
            if (i.view.world) refresh_map_world_hover(*i.view.world);
            {
                InputScope scope(i.view.terrain_loaded);
                process_mouse_input(i.event, i.result);
            }
            runtime_log_event("input_result", "kind=%u consumed=%d warp=%d pointer=%d,%d camera=%d,%d",
                              unsigned(i.event.kind), i.result.consumed, i.result.warp_pointer, i.result.pointer_x,
                              i.result.pointer_y, TacticalClass::Instance->TacticalPos.X,
                              TacticalClass::Instance->TacticalPos.Y);
            if (i.result.warp_pointer) i.view.pointer = {i.result.pointer_x, i.result.pointer_y};
            tooltip_input(i.event);
            if (before != TacticalClass::Instance->TacticalPos) ++i.view.camera_generation;
            if (i.view.world) refresh_map_world_hover(*i.view.world);
            ++i.view.ui_generation;
        },
        &input);
}
namespace {
struct AdvanceView {
    MapViewHandle& view;
    const MapDrawingContext* drawing;
    MapDrawStatistics* stats;
    DrawingStatus result = DrawingStatus::skipped;
    bool rendered = false;
    bool batch = false;
    unsigned iterations = 0;
    double drawing_ms = 0;
};
// Input phase: the device state of this host. The core operation is the original
// GScreenClass::GetInputAndUpdate followed by Game::KeyboardProcess.
bool input_hook(void* p, void (*operation)()) {
    auto& v = static_cast<AdvanceView*>(p)->view;
    InputScope scope(v.terrain_loaded);
    if (current_game_loop()->poll_input)
        update_mouse_input(v.pointer, v.pointer_inside, operation);
    // Retain frame-based sidebar/credit updates without repeating device
    // gestures and camera scrolling on intermediate high-speed iterations.
    else if (HouseClass::CurrentPlayer)
        TabClass::Instance.Update(0, v.pointer_inside ? v.pointer : Point2D{-1, -1});
    return true;
}
// Render phase: the drawing target. The operation is GScreenClass::Render.
bool render_hook(void* p, void (*operation)()) {
    auto& a = *static_cast<AdvanceView*>(p);
    const bool presenting = a.drawing && current_game_loop()->present;
    render_phase_before_draw(a.view, presenting);
    if (!presenting) {
        // Headless update retains the same presentation phase and pause behavior.
        render_phase_after_draw(a.view, false);
        return true;
    }
    set_drawing_diagnostic_frame(Unsorted::CurrentFrame);
    const auto started = debug_performance_time();
    a.result = draw_game_view_bound(a.view, *a.drawing, *a.stats, operation);
    draw_coverage_frame_end();
    recorder_frame_end();
    const auto finished = debug_performance_time();
    a.drawing_ms += (finished - started) * 1000;
    a.view.debug.performance.drawing.add(finished, (finished - started) * 1000);
    a.rendered = a.result == DrawingStatus::drawn || a.result == DrawingStatus::skipped;
    if (!a.rendered) {
        record_drawing_failure(a.result, "core_draw");
        set_error(a.view, drawing_failure());
    }
    render_phase_after_draw(a.view, true);
    return a.rendered;
}
// Frame end: the presentation bookkeeping that follows every original phase.
bool frame_end_hook(void* p) {
    auto& v = static_cast<AdvanceView*>(p)->view;
    tooltip_poll_timer();
    if (v.world) end_map_world_frame(*v.world);
    return true;
}
// One binding of the view services per iteration that actually runs. The logic time
// of an iteration that advanced the frame is all of it except its drawing.
bool bind_hook(void* p, void (*operation)()) {
    auto& a = *static_cast<AdvanceView*>(p);
    ++a.iterations;
    const int frame = Unsorted::CurrentFrame;
    const double drawing = a.drawing_ms, started = debug_performance_time();
    const bool bound = with_map_view(a.view, [](void* q) { (*static_cast<void (**)()>(q))(); }, &operation);
    if (Unsorted::CurrentFrame != frame) {
        const double finished = debug_performance_time();
        a.view.debug.performance.logic.add(finished, (finished - started) * 1000 - (a.drawing_ms - drawing));
    }
    return bound;
}
bool advance_view(AdvanceView& a, double seconds) noexcept {
    auto& view = a.view;
    if (active != &view || view.operation_depth || !view.terrain_loaded) return false;
    GameLoopContext loop{view.loop, view.scenario.unknown_62C != 0, &a, input_hook, render_hook, frame_end_hook};
    loop.scenario = &view.scenario;
    loop.bind = bind_hook;
    loop.render_operation = present_game_screen;
    const auto before = view.loop.iterations;
    const auto camera_before = TacticalClass::Instance->TacticalPos;
    const auto* previous_tip = view.tooltips.CurrentToolTip;
    const bool result = a.batch ? advance_game_loop_batch(loop, seconds) : advance_game_loop(loop, seconds);
    if (result && seconds > 0) {
        const auto now = debug_performance_time();
        auto& perf = view.debug.performance;
        perf.frames.add(now, seconds);
        perf.steps.add(now, double(view.loop.iterations - before));
        perf.limited.add(now, view.loop.batch_limited ? 1 : 0);
    }
    if (!result) return false;
    if (view.loop.result && view.transition.state == MissionTransitionState::none) {
        static std::atomic<std::uint64_t> next_result{1};
        auto& transition = view.transition;
        transition.id = next_result.fetch_add(1);
        transition.result = static_cast<GameResult>(view.loop.result);
        transition.state = MissionTransitionState::unavailable;
        std::snprintf(transition.filename, sizeof(transition.filename), "%s", view.loaded_filename);
        const bool prepared = with_map_view(view, [](void* p) {
            auto& v = *static_cast<MapViewHandle*>(p);
            auto& t = v.transition;
            v.next_campaign = v.scenario.CampaignIndex;
            if (t.result == GameResult::defeat) {
                t.state = MissionTransitionState::retry;
                return;
            }
            if (SessionClass::Instance.GameMode != GameMode::Campaign || v.scenario.EndOfGame ||
                v.scenario.OneTimeOnly) {
                t.state = MissionTransitionState::complete;
                return;
            }
            char old_name[sizeof(v.scenario.FileName)];
            std::memcpy(old_name, v.scenario.FileName, sizeof(old_name));
            const short old_stage = v.scenario.Stage;
            struct RestoreSelection {
                ScenarioClass& scenario;
                const char* name;
                short stage;
                ~RestoreSelection() {
                    std::memcpy(scenario.FileName, name, sizeof(scenario.FileName));
                    scenario.Stage = stage;
                }
            } restore{v.scenario, old_name, old_stage};
            if (v.scenario.PrepareNextScenario() && v.scenario.FileName[0] &&
                std::strcmp(old_name, v.scenario.FileName)) {
                std::snprintf(t.filename, sizeof(t.filename), "%s", v.scenario.FileName);
                v.next_stage = v.scenario.Stage;
                t.state = MissionTransitionState::next;
            } else
                std::snprintf(t.error, sizeof(t.error), "No valid campaign successor for %.260s", old_name);
        }, &view);
        if (!prepared) std::snprintf(transition.error, sizeof(transition.error), "%.511s", view.error);
    }
    // Presentation generations observe the state after the step, not each phase.
    if (a.iterations) {
        auto& ui = view.ui_generation;
        if (previous_tip != view.tooltips.CurrentToolTip) ++ui;
        if (PowerClass::Instance.PowerNeedRedraw) ++ui;
        if (RadarClass::Instance.unknown_bool_14D9 || RadarClass::Instance.unknown_points_125C.Count) ++ui;
        // Scripted and requested view moves, and CenterCameraAtWaypoint from trigger AI.
        if (camera_before != TacticalClass::Instance->TacticalPos) {
            ++view.camera_generation;
            ++ui;
        }
        if (HouseClass::CurrentPlayer &&
            TabClass::Instance.TabData.LastValue != HouseClass::CurrentPlayer->Available_Money())
            ++ui;
        if (RadarEventClass::Array.Count || SidebarClass::Instance.SidebarNeedsRedraw ||
            SidebarClass::Instance.Tabs[SidebarClass::Instance.ActiveTabIndex].NeedsRedraw)
            ++ui;
    }
    // Suspension and parameter upkeep run once per host update; the audio AI itself
    // runs inside the main loop (Game::CallBack).
    update_native_audio(Game::IsFocused, effective_audio_paused(view));
    return true;
}
DrawingStatus advance_drawn_view(MapViewHandle& view, double seconds, const MapDrawingContext& drawing,
                                 MapDrawStatistics& stats, bool& rendered, bool batch) noexcept {
    stats = {};
    rendered = false;
    if (!valid_drawing_context(drawing.types))
        return DrawingStatus::unsupported;
    clear_drawing_failure();
    view.error[0] = 0;
    prepare_map_shapes(view, drawing);
    AdvanceView a{view, &drawing, &stats};
    a.batch = batch;
    if (!advance_view(a, seconds))
        return a.result == DrawingStatus::drawn || a.result == DrawingStatus::skipped ? DrawingStatus::backend_failure
                                                                                      : a.result;
    rendered = a.rendered;
    if (!drawing_completed(a.result))
        set_error(view, drawing_failure()[0] ? drawing_failure() : drawing_status_name(a.result));
    return a.result;
}
}  // namespace
bool update_game_view(MapViewHandle& view, double seconds) noexcept {
    AdvanceView a{view, nullptr, nullptr};
    return advance_view(a, seconds);
}
bool update_game_view_batch(MapViewHandle& view, double seconds) noexcept {
    AdvanceView a{view, nullptr, nullptr};
    a.batch = true;
    return advance_view(a, seconds);
}
DrawingStatus advance_game_view(MapViewHandle& view, double seconds, const MapDrawingContext& drawing,
                                MapDrawStatistics& stats, bool& rendered) noexcept {
    return advance_drawn_view(view, seconds, drawing, stats, rendered, false);
}
DrawingStatus advance_game_view_batch(MapViewHandle& view, double seconds, const MapDrawingContext& drawing,
                                      MapDrawStatistics& stats, bool& rendered) noexcept {
    return advance_drawn_view(view, seconds, drawing, stats, rendered, true);
}
bool set_game_speed_level(MapViewHandle& view, int level) noexcept {
    if (active != &view || view.operation_depth) return false;
    runtime_log_event("speed_request", "level=%d", level);
    if (!set_game_loop_speed(view.loop, level)) return false;
    view.debug.original_speed = 0;
    return true;
}
bool get_game_speed_level(const MapViewHandle& view, int& level) noexcept {
    if (active != &view || view.operation_depth) return false;
    level = view.loop.speed_level;
    return true;
}
bool set_game_view_activity(MapViewHandle& view, bool focused, bool paused) noexcept {
    if (active != &view || view.operation_depth || current_game_loop()) return false;
    const bool changed = Game::IsFocused != focused || view.loop.host_paused != paused;
    if (paused && !view.loop.host_paused && Game::SpecialDialog == 1) {
        if (!with_map_view(view, [](void*) { ScenarioClass::PauseGame(); }, nullptr)) return false;
        view.options_paused = true;
    }
    if (!paused && view.loop.host_paused && view.options_paused) {
        if (!with_map_view(view, [](void*) { ScenarioClass::ResumeGame(); }, nullptr)) return false;
        view.options_paused = false;
        Game::SpecialDialog = 0;
    }
    if (view.loop.host_paused != paused) reset_game_loop_schedule(view.loop);
    view.loop.host_paused = paused;
    Game::IsFocused = focused;
    if (changed)
        runtime_log_event("activity", "focused=%d paused=%d speed_level=%d", focused, paused, view.loop.speed_level);
    // Apply pause immediately when the host stops issuing update callbacks.
    suspend_native_audio(effective_audio_paused(view));
    // A movie may itself pause Scenario logic; only host pause stops its clock.
    set_native_movie_activity(!view.loop.host_paused);
    return true;
}
bool get_game_view_state(const MapViewHandle& view, GameViewState& output) noexcept {
    if (active != &view || view.operation_depth) return false;
    output = {view.ui_generation,
              effective_audio_paused(view),
              RadarClass::Instance.IsAvailableNow,
              GadgetClass::StuckOn != nullptr || MouseClass::Instance.unknown_byte_554A != 0,
              SidebarClass::Instance.ActiveTabIndex,
              TabClass::Instance.ThumbActive};
    output.current_frame = Unsorted::CurrentFrame;
    output.game_speed = GameOptionsClass::Instance.GameSpeed;
    output.logic_iterations = view.loop.iterations;
    output.elapsed_wall_seconds = view.loop.elapsed_seconds;
    output.focused = Game::IsFocused;
    output.speed_level = view.loop.speed_level;
    output.target_logic_fps = game_loop_target_rate(view.loop);
    output.batch_steps = view.loop.batch_steps;
    output.batch_limited = view.loop.batch_limited;
    output.special_dialog = Game::SpecialDialog;
    output.result = view.loop.result ? static_cast<GameResult>(view.loop.result)
                    : Game::PlayerWins ? GameResult::victory
                    : Game::PlayerLoses ? GameResult::defeat : GameResult::in_progress;
    return true;
}
const char* map_view_error(const MapViewHandle& view) noexcept {
    return active == &view ? view.error : "Invalid map view";
}
}
