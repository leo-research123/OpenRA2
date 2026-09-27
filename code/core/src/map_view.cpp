#include "map_view.hpp"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/MessageListClass.h"
#include "tactical_drawing.hpp"
#include "type_drawing.hpp"
#include "map_world.hpp"
#include "map_configuration.hpp"
#include "api/filesystem.hpp"
#include "scenario_runtime.hpp"
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
#include <bit>
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
// 6D1830/6D18A0/6D18C0 and the original quantized sine table. Reproduce
// float inputs to 5AEF60/5AF1A0, then the scale input to 5AEA10.
constexpr float sin_x = std::bit_cast<float>(0x3f5dab76u);
constexpr float cos_x = std::bit_cast<float>(0x3f000e82u);
constexpr float sin_z = std::bit_cast<float>(0x3f3504f3u);
constexpr float cos_z = std::bit_cast<float>(0x3f3504f3u);
constexpr float scale = std::bit_cast<float>(0x3e29b4a4u);
void set_error(MapViewHandle& view, const char* message) noexcept {
    std::snprintf(view.error, sizeof(view.error), "%s", message);
}
void release_world(MapViewHandle& view) noexcept {
    SidebarClass::Instance.IsSidebarActive=false;
    view.world.reset(); // Objects detach while cells and borrowed type art still live.
    // Original world-object teardown ends with the beacon manager (0x53499B).
    // Empty/failed sessions also retire any remaining owned beacon slots.
    BeaconManagerClass::Instance.Reset();
    RadarEventClass::Clear();
    MouseClass::Instance.ResetInput();
    MessageListClass::Instance.Init(0,0,0,0,0,0,0,0,0,0);
    SidebarClass::Instance.SidebarClass::Init_Clear();
    GScreenClass::Buttons=nullptr;
    RadarClass::RadarButton.Zap();
    RadarClass::DiplomacyButton.Zap();
    RadarClass::OptionsButton.Zap();
    SidebarClass::ToggleRepairButton.Zap();SidebarClass::ToggleRepairButton.SetShape(nullptr,0,0);
    SidebarClass::ToggleSellButton.Zap();SidebarClass::ToggleSellButton.SetShape(nullptr,0,0);
    for (auto& tab : SidebarClass::TabButtons) { tab.Zap(); tab.SetShape(nullptr,0,0); }
    for (int i=0;i<240;++i) SelectClass::Array()[i].Zap();
    for (auto& button : TabClass::CommandButtons) { button.Zap(); button.SetShape(nullptr,0,0); }
    for (auto* button : {&TabClass::CollapseButton,&TabClass::ExpandButton}) { button->Zap(); button->SetShape(nullptr,0,0); }
    TabClass::Instance.ThumbActive=true;
    view.tooltips.SetState(false);
    while(view.tooltips.ToolTips.Count)view.tooltips.Remove(view.tooltips.ToolTips[0]->GadgetID);
    view.tooltip_platform.timer_owner=nullptr;
    view.keyboard.Reset(); view.pointer_inside=false; view.loop=GameLoopState{};
    view.ui_resources.reset();
    MixFileClass::UnloadSidebarMixes();
    RadarClass::Instance.ReleaseTerrainRadar();
    MapClass::Instance.ReleaseCellStorage();
    IsometricTileTypeClass::ClearTileSetCatalog();
    while (AnimTypeClass::Array.Count>view.animation_baseline)
        GameDelete(AnimTypeClass::Array[AnimTypeClass::Array.Count-1]);
    Theater::UnmountResourceMixes();
    view.terrain_loaded=false;
    TacticalClass::ViewBounds={};
    DSurface::ViewBounds=DSurface::SidebarBounds=DSurface::WindowBounds={};
}
void reset_world(MapViewHandle& view) {
    release_world(view);
    if (view.rules) { view.rules=std::make_unique<RulesClass>(); RulesClass::Instance=view.rules.get(); }
    Unsorted::CurrentFrame=0;
    view.scenario.Reset();
    view.scenario.Random=Randomizer(0);
    view.tactical.~TacticalClass();
    ::new (&view.tactical) TacticalClass({}, {}, sin_x, cos_x, sin_z, cos_z, scale);
}
}
MapViewHandle::MapViewHandle(ResourceHandle& source)
    : resources(&source), tactical({}, {}, sin_x, cos_x, sin_z, cos_z, scale),
      rules(RulesClass::Instance ? nullptr : std::make_unique<RulesClass>()),
      animation_baseline(AnimTypeClass::Array.Count) {}
MapViewHandle::~MapViewHandle() = default;
bool create_map_view(ResourceHandle& resources, MapViewHandle*& output) noexcept {
    if (output || active || ScenarioClass::Instance || TacticalClass::Instance || MapClass::Instance.Cells.Items ||
        IsometricTileTypeClass::Array.Count) return false;
    try {
        auto view = std::make_unique<MapViewHandle>(resources);
        output = view.release(); active = output;
        return true;
    } catch (...) { return false; }
}
bool with_map_view(MapViewHandle& view, void (*operation)(void*), void* argument) noexcept {
    if (active != &view || !operation) return false;
    if (ScenarioClass::Instance && ScenarioClass::Instance != &view.scenario) {
        set_error(view,"Another Scenario owns the active world"); return false;
    }
    ClockReadScope clock_scope;
    const bool previous_focus=Game::IsFocused;
    Game::IsFocused=view.loop.focused;
    struct RestoreFocus {bool value;~RestoreFocus(){Game::IsFocused=value;}} restore_focus{previous_focus};
    view.tooltip_platform.pointer=&view.pointer;
    view.tooltip_platform.pointer_visible=&view.pointer_inside;
    ToolTipScope tooltip_scope(view.tooltip_platform,view.tooltips);
    view.tooltips.SetState(GameOptionsClass::Instance.Tooltips);
    // Zero-delay mouse messages and headless timer delivery also measure text.
    // The resource session owns GAME.FNT; its original global is only borrowed.
    auto* previous_font=BitFont::Instance;
    if(view.ui_resources && view.ui_resources->font())BitFont::Instance=view.ui_resources->font();
    struct RestoreFont {BitFont* old;~RestoreFont(){BitFont::Instance=old;}} restore_font{previous_font};
    ++view.operation_depth;
    struct EndOperation { MapViewHandle& view; ~EndOperation() { --view.operation_depth; } } end{view};
    struct Context { MapViewHandle& view; void (*operation)(void*); void* argument; bool completed = false; } context{view,operation,argument};
    try {
        std::string error;
        const bool result = with_resources(*view.resources, [](void* pointer) {
            auto& context = *static_cast<Context*>(pointer);
            auto* old_rules=RulesClass::Instance;
            if (context.view.rules) RulesClass::Instance=context.view.rules.get();
            struct RestoreRules { RulesClass* old; ~RestoreRules(){RulesClass::Instance=old;} } restore_rules{old_rules};
            auto* old_input=InputManagerClass::Instance;
            InputManagerClass::Instance=&context.view.keyboard;
            struct RestoreInput { InputManagerClass* old; ~RestoreInput(){InputManagerClass::Instance=old;} } restore_input{old_input};
            auto* old = ScenarioClass::Instance;
            ScenarioClass::Instance = &context.view.scenario;
            struct Restore { ScenarioClass* old; ~Restore() { ScenarioClass::Instance = old; } } restore{old};
            auto services = default_scenario_runtime();
            ScenarioHouseServices houses{};
            houses.session=&context.view.session;
            houses.current_player=&HouseClass::CurrentPlayer;
            services.houses=&houses;
            ScenarioRenderServices rendering{};
            rendering.color_schemes=&ColorScheme::Array;
            services.render=&rendering;
            context.view.session.GameMode=context.view.loop.mode;
            services.context = &context.view;
            services.session_mode = [](void* p) noexcept {
                return static_cast<int>(static_cast<MapViewHandle*>(p)->loop.mode);
            };
            services.variable_changed = [](void*, bool global, int index) {
                if(global)TagClass::NotifyGlobalChanged(index);else TagClass::NotifyLocalChanged(index);
            };
            services.cell_at = [](void*, const CellStruct& coordinate, CellClass*& output) {
                output = MapClass::Instance.GetCellAt(coordinate);
                return output != nullptr;
            };
            context.completed = with_scenario_runtime(services, [](void* p) {
                auto& c=*static_cast<Context*>(p);
                if(!with_map_world(c.view.world.get(),c.operation,c.argument))
                    throw std::runtime_error("Native map-world operation failed");
            }, &context);
        }, &context, error);
        if (!result || !context.completed) {
            set_error(view, error.empty() ? "Map operation unavailable" : error.c_str());
            return false;
        }
        view.error[0] = 0;
        return true;
    } catch (const std::exception& error) { set_error(view,error.what()); return false; }
    catch (...) { set_error(view,"Map operation failed"); return false; }
}
bool initialize_empty_map_view(MapViewHandle& view, const RectangleStruct& bounds, char level) noexcept {
    if (active != &view) return false;
    if (view.operation_depth) { set_error(view,"Cannot reload a map during an active operation"); return false; }
    view.state = MapViewState::empty;
    ++view.generation;
    struct Context { MapViewHandle& view; RectangleStruct bounds; char level; bool complete = false; } context{view,bounds,level};
    const bool scoped = with_map_view(view, [](void* pointer) {
        auto& context = *static_cast<Context*>(pointer);
        reset_world(context.view);
        MapClass::Instance.MaxLevel=13;
        context.complete = MapClass::Instance.CreateEmptyCells(context.bounds, context.level);
    }, &context);
    if (!scoped || !context.complete) {
        MapClass::Instance.ReleaseCellStorage();
        if (scoped) set_error(view,"Could not allocate the requested map dimensions or level");
        view.state = MapViewState::failed;
        return false;
    }
    view.state = MapViewState::ready;
    return true;
}
bool load_map_view(MapViewHandle& view, const char* filename, std::uint32_t length) noexcept {
    if (active!=&view) return false;
    if (view.operation_depth) { set_error(view,"Cannot reload a map during an active operation"); return false; }
    if (!filename || !length || length>=sizeof(view.scenario.FileName) || std::memchr(filename,0,length)) {
        set_error(view,"Invalid map filename"); return false;
    }
    // Capture before teardown: the caller may borrow the current Scenario name.
    char name[260]{}; std::memcpy(name,filename,length);
    view.state=MapViewState::empty; ++view.generation;
    struct Context { MapViewHandle& view; const char* filename; const char* failure="Map load failed"; bool complete=false; char detail[512]{}; } context{view,name};
    const bool scoped=with_map_view(view,[](void* pointer) {
        auto& c=*static_cast<Context*>(pointer);
        reset_world(c.view);
        struct Rollback { Context& c; ~Rollback() { if (!c.complete) release_world(c.view); } } rollback{c};
        c.failure="Could not mount map packages";
        if (!MixFileClass::LoadMapMixes()) return;
        c.failure="Could not load original KEYBOARDMD.INI command bindings";
        if(!initialize_player_commands())return;
        c.view.world=std::make_unique<MapWorld>(c.view);
        auto& ini=c.view.world->map_ini;
        c.failure="Map file not found in the resource directory or mounted MIX packages";
        CCFileClass file(c.filename);
        if (!file.Exists()) return;
        c.failure="Could not read map INI";
        if (ini.ReadCCFile(&file,false,false)<=0) return;
        c.failure="Map INI is missing the [Map] section";
        if (!ini.GetSection("Map")) return;
        c.view.loop.mode=ini.ReadBool("Basic","MultiplayerOnly",false)?GameMode::Skirmish:GameMode::Campaign;
        c.view.session.GameMode=c.view.loop.mode;
        auto& scenario=c.view.scenario;
        std::strcpy(scenario.FileName,c.filename);
        scenario.Theater=static_cast<TheaterType>(ini.ReadTheater("Map","Theater",0));
        c.failure="Could not mount theater resources";
        if (!Theater::MountResourceMixes(scenario.Theater)) return;
        c.failure="Could not load theater palettes";
        if (!FileSystem::LoadTheaterPalettes(scenario.Theater)) return;
        c.failure="Could not load map object types and art";
        if(!load_map_world_types(*c.view.world,ini,c.detail,sizeof(c.detail))) {
            c.failure=c.detail;
            return;
        }
        if(!parse_map_scenario_fields(*c.view.world,c.detail,sizeof(c.detail))) {
            c.failure=c.detail;
            return;
        }
        c.failure="Could not load map terrain, theater catalog, packed cells or referenced TMP resources";
        GameOptionsClass::Instance.DetailLevel=2;
        CCFileClass options_file("RA2MD.INI"); CCINIClass options_ini;
        if (options_file.Exists() && options_ini.ReadCCFile(&options_file,false,false)>0) {
            auto& options=GameOptionsClass::Instance;
            options.DetailLevel=std::clamp(options_ini.ReadInteger("Options","DetailLevel",2),0,2);
            options.ScrollRate=std::clamp(options_ini.ReadInteger("Options","ScrollRate",3),0,7);
            options.ScrollMethod=std::clamp(options_ini.ReadInteger("Options","ScrollMethod",0),0,2);
            options.AutoScroll=options_ini.ReadBool("Options","AutoScroll",true);
            options.Tooltips=options_ini.ReadBool("Options","ToolTips",true);
        }
        if (!DisplayClass::Instance.LoadTerrainFromINI(ini)) return;
        c.failure="Could not instantiate native map objects";
        struct Objects { MapWorld& world; CCINIClass& ini; bool ok=false; } objects{*c.view.world,ini};
        if(!with_map_world(c.view.world.get(),[](void* p){
            auto& o=*static_cast<Objects*>(p);o.ok=load_map_world_objects(o.world,o.ini);
        },&objects)||!objects.ok)return;
        Theater::LastTheater=scenario.Theater;
        // Explicit terrain-browser startup: reveal canonical Cell flags.
        // Real scenario shroud/reveal sources will own these flags in a match.
        for (int i=0;i<MapClass::Instance.Cells.Capacity;++i)
            if (auto* cell=MapClass::Instance.Cells[i]) cell->AltFlags|=AltCellFlags::Clear;
        // A narrow/empty radar fit or allocation failure must not discard an
        // otherwise usable map. Its absence is exposed by the radar query API.
        RadarClass::Instance.BuildTerrainRadar();
        if(auto* player=HouseClass::CurrentPlayer) {
            player->UpdatePower();
            player->UpdateRadarAvailability();
        }
        c.view.terrain_loaded=true;
        c.complete=true;
    },&context);
    if (!scoped || !context.complete) {
        if (context.detail[0]) set_error(view,context.detail);
        else if (scoped) set_error(view,context.failure);
        view.state=MapViewState::failed;
        return false;
    }
    view.state=MapViewState::ready;
    return true;
}
void destroy_map_view(MapViewHandle*& view) noexcept {
    if (!view) return;
    if (view != active) return;
    if (view->operation_depth) { set_error(*view,"Cannot close a map during an active operation"); return; }
    // Stop the world's consumers before calling this boundary. Cells release
    // their borrowed resources while the resource environment is still alive.
    if (!with_map_view(*view, [](void* pointer) { release_world(*static_cast<MapViewHandle*>(pointer)); }, view))
        release_world(*view);
    active = nullptr;
    delete view;
    view = nullptr;
}
bool get_map_view_info(const MapViewHandle& view, MapViewInfo& output) noexcept {
    if (active != &view) return false;
    const auto& map = MapClass::Instance;
    output = {view.state,view.generation,map.MapRect.Width,map.MapRect.Height,map.Cells.Capacity,
        view.terrain_loaded,static_cast<int>(view.scenario.Theater),map.VisibleRect.X,map.VisibleRect.Y,
        map.VisibleRect.Width,map.VisibleRect.Height,TacticalClass::ViewBounds.Width,TacticalClass::ViewBounds.Height,
        view.tactical.TacticalPos.X,view.tactical.TacticalPos.Y,view.camera_generation};
    if(view.world){output.presentation_revision=view.world->presentation_revision;
        output.resource_revision=view.world->resource_revision;output.simulation_tick=view.world->simulation_tick;}
    return true;
}
bool set_map_viewport(MapViewHandle& view,int width,int height) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded || width<1 || height<1 || width>8192 || height>8192) return false;
    if (TacticalClass::ViewBounds.Width==width && TacticalClass::ViewBounds.Height==height) return true;
    Point2D center;
    if (!TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height) {
        auto& scenario=view.scenario;
        CellStruct home{static_cast<short>((scenario.Width+scenario.Height)/2),static_cast<short>((scenario.Width+scenario.Height)/2)};
        if (scenario.HomeCell>=0 && scenario.HomeCell<702 && scenario.IsDefinedWaypoint(scenario.HomeCell)) home=scenario.Waypoints[scenario.HomeCell];
        const auto* cell=MapClass::Instance.TryGetCellAt(home);
        center=TacticalClass::CoordsToScreen(CoordStruct{int(home.X)*256+128,int(home.Y)*256+128,0});
        center.Y-=cell ? 15*int(static_cast<signed char>(cell->Level)) : 0;
    } else {
        center={view.tactical.TacticalPos.X+TacticalClass::ViewBounds.Width/2,
            view.tactical.TacticalPos.Y+TacticalClass::ViewBounds.Height/2};
    }
    TacticalClass::ViewBounds={0,0,width,height};
    return center_map_view(view,center.X,center.Y);
}
bool set_game_view_size(MapViewHandle& view,int width,int height) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded ||
        width<640 || height<480 || width>8192 || height>8192) return false;
    struct Resize { MapViewHandle& view; int width,height; } resize{view,width,height};
    return with_map_view(view,[](void* pointer) {
        const auto& r=*static_cast<Resize*>(pointer);
        auto& view=r.view;
        const auto before=view.tactical.TacticalPos;
        if (!TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height) {
            auto& scenario=view.scenario;
            CellStruct home{static_cast<short>((scenario.Width+scenario.Height)/2),
                static_cast<short>((scenario.Width+scenario.Height)/2)};
            if (scenario.HomeCell>=0 && scenario.HomeCell<702 && scenario.IsDefinedWaypoint(scenario.HomeCell))
                home=scenario.Waypoints[scenario.HomeCell];
            auto center=TacticalClass::CoordsToScreen({int(home.X)*256+128,int(home.Y)*256+128,0});
            if (const auto* cell=MapClass::Instance.TryGetCellAt(home))
                center.Y-=15*int(static_cast<signed char>(cell->Level));
            view.tactical.TacticalCoord1=center;
        }
        DSurface::WindowBounds={0,0,r.width,r.height};
        DisplayClass::Instance.Set_View_Dimensions({0,0,r.width-168,r.height-32});
        // Original scenario/display startup calls Tab.Activate(1) at
        // 0x0067E69F / 0x00561113; Sidebar.Activate writes this flag at
        // 0x006A7DA6. Native layout and gadget registration are separate,
        // but the visible sidebar must publish the same activation state.
        SidebarClass::Instance.IsSidebarActive=true;
        if (before!=view.tactical.TacticalPos) ++view.camera_generation;
    },&resize);
}
bool get_game_view_layout(MapViewHandle& view,GameViewLayout& output) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded || DSurface::WindowBounds.Width<640) return false;
    GameViewLayout result;
    struct Query { MapViewHandle& view; GameViewLayout& result; } query{view,result};
    if (!with_map_view(view,[](void* pointer) {
        auto& q=*static_cast<Query*>(pointer);
        auto& sidebar=SidebarClass::Instance;
        q.result={DSurface::WindowBounds,DSurface::ViewBounds,DSurface::SidebarBounds,
            RadarClass::Instance.GetPanelBounds(),TabClass::Instance.GetCommandBarBounds(),
            sidebar.CameoPosition,sidebar.CameoPitch,sidebar.ScrollPosition,q.view.scenario.PlayerSideIndex,
            sidebar.GetUsableCameoCount(),sidebar.GetVisibleCameoCount()};
    },&query)) return false;
    output=result;
    return true;
}
bool center_map_view(MapViewHandle& view,int x,int y) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height) return false;
    const auto before=view.tactical.TacticalPos;
    if (!view.tactical.FocusView({x,y})) return false;
    if (before!=view.tactical.TacticalPos) ++view.camera_generation;
    return true;
}
bool scroll_map_view(MapViewHandle& view,int dx,int dy) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height) return false;
    const auto& center=view.tactical.TacticalCoord1;
    const auto bounded=[](std::int64_t value) { return static_cast<int>(std::clamp<std::int64_t>(value,INT_MIN,INT_MAX)); };
    return center_map_view(view,bounded(std::int64_t(center.X)+dx),bounded(std::int64_t(center.Y)+dy));
}
bool get_map_radar_info(MapViewHandle& view,MapRadarInfo& output) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded) return false;
    auto& radar=RadarClass::Instance;
    if (!radar.unknown_123C) return false;
    const auto& rect=radar.unknown_rect_149C;
    CellStruct center;
    const bool valid=TacticalClass::ViewBounds.Width>0 && TacticalClass::ViewBounds.Height>0 &&
        view.tactical.PickTerrainCell({TacticalClass::ViewBounds.Width/2,TacticalClass::ViewBounds.Height/2},TacticalClass::ViewBounds,center) &&
        radar.UpdateViewportFrame(center,{TacticalClass::ViewBounds.Width,TacticalClass::ViewBounds.Height});
    const auto frame=valid ? radar.unknown_rect_14DC : RectangleStruct{};
    // The host minimap API remains panel-local; original Radar fields are
    // sidebar-local. Convert only here, at the host boundary.
    const int x=int(radar.unknown_11F0),y=int(radar.unknown_11F4);
    output={140,108,rect.X-x,rect.Y-y,rect.Width,rect.Height,int(radar.unknown_1240),int(radar.unknown_1244),
        valid ? frame.X-x : 0,valid ? frame.Y-y : 0,frame.Width,frame.Height,valid};
    return true;
}
bool copy_map_radar_pixels(MapViewHandle& view,std::uint16_t* output,std::uint32_t capacity) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded) return false;
    const auto& radar=RadarClass::Instance;
    return radar.CopyTerrainRadar(output,capacity);
}
bool center_map_from_radar(MapViewHandle& view,int x,int y) noexcept {
    if (active!=&view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height) return false;
    const auto& radar=RadarClass::Instance; const auto& rect=radar.unknown_rect_149C;
    x+=int(radar.unknown_11F0); y+=int(radar.unknown_11F4);
    if (!radar.unknown_123C || x<rect.X || y<rect.Y || x>=rect.X+rect.Width || y>=rect.Y+rect.Height) return false;
    CellStruct target;
    if (!radar.RadarToTerrainCell({x,y},target)) return false;
    CoordStruct world;
    MapClass::Instance.GetCellAt(target)->GetCellCoords(&world);
    const auto center=TacticalClass::CoordsToScreen(world);
    return center_map_view(view,center.X,center.Y);
}
DrawingStatus draw_map_view(MapViewHandle& view,const MapDrawingContext& drawing,MapDrawStatistics& output) noexcept {
    output={};
    clear_drawing_failure();
    if (active!=&view || view.operation_depth || !view.terrain_loaded || !TacticalClass::ViewBounds.Width || !TacticalClass::ViewBounds.Height)
        return DrawingStatus::unavailable;
    struct Frame { MapViewHandle& view; const MapDrawingContext& drawing; MapDrawStatistics& stats; DrawingStatus result=DrawingStatus::unavailable; } frame{view,drawing,output};
    if (!with_map_view(view,[](void* pointer) {
        auto& frame=*static_cast<Frame*>(pointer);
        frame.result=draw_tactical_view(frame.view.tactical,frame.view.world.get(),frame.drawing,TacticalClass::ViewBounds,frame.stats);
    },&frame)) {
        if(drawing_failure()[0])set_error(view,drawing_failure());
        return DrawingStatus::backend_failure;
    }
    if (frame.result!=DrawingStatus::drawn && frame.result!=DrawingStatus::skipped)
        set_error(view,drawing_failure()[0]?drawing_failure():drawing_status_name(frame.result));
    return frame.result;
}
DrawingStatus draw_game_view(MapViewHandle& view,const MapDrawingContext& drawing,MapDrawStatistics& output) noexcept {
    output={};
    clear_drawing_failure();
    if (active!=&view || view.operation_depth || !view.terrain_loaded || DSurface::WindowBounds.Width<640)
        return DrawingStatus::unavailable;
    struct Frame {
        MapViewHandle& view; const MapDrawingContext& drawing; MapDrawStatistics& statistics;
        DrawingStatus result=DrawingStatus::unavailable;
        char failure[128]{};
    } frame{view,drawing,output};
    if (!with_map_view(view,[](void* pointer) {
        auto& f=*static_cast<Frame*>(pointer);
        if (!f.view.ui_resources) f.view.ui_resources=std::make_unique<UiResources>();
        auto& resources=*f.view.ui_resources;
        if (!resources.load(f.view.scenario.PlayerSideIndex)) {
            std::snprintf(f.failure,sizeof(f.failure),"%s",resources.error()); return;
        }
        GameUiFrame context{f.drawing,resources,f.statistics};
        context.advance_presentation=current_game_loop()!=nullptr;
        f.result=with_game_ui_frame(context,[] { GScreenClass::Instance.Render(); });
    },&frame)) {
        if(drawing_failure()[0])set_error(view,drawing_failure());
        return DrawingStatus::backend_failure;
    }
    if (frame.result!=DrawingStatus::drawn && frame.result!=DrawingStatus::skipped)
        set_error(view,frame.failure[0] ? frame.failure : drawing_failure()[0]?drawing_failure():drawing_status_name(frame.result));
    return frame.result;
}
bool submit_game_input(MapViewHandle& view,const GameInputEvent& event,GameInputResult& result) noexcept {
    result={};
    if (active!=&view || view.operation_depth || !view.terrain_loaded ||
        static_cast<unsigned>(event.kind)>static_cast<unsigned>(GameInputKind::focus_gained) ||
        event.x < -65536 || event.x > 65536 || event.y < -65536 || event.y > 65536 || event.code>255) return false;
    struct Input { MapViewHandle& view; const GameInputEvent& event; GameInputResult& result; } input{view,event,result};
    if(event.kind==GameInputKind::focus_gained){view.loop.focused=true;return true;}
    if(event.kind==GameInputKind::focus_lost)view.loop.focused=false;
    return with_map_view(view,[](void* pointer) {
        auto& i=*static_cast<Input*>(pointer);
        const auto before=i.view.tactical.TacticalPos;
        MouseClass::Instance.ProcessInput(i.event,i.result);
        if (i.event.kind==GameInputKind::focus_lost || i.event.kind==GameInputKind::pointer_leave) i.view.pointer_inside=false;
        else if (i.event.kind!=GameInputKind::key) {
            i.view.pointer={i.event.x,i.event.y};
            const auto& canvas=DSurface::WindowBounds.Width>0?DSurface::WindowBounds:TacticalClass::ViewBounds;
            i.view.pointer_inside=i.event.x>=0 && i.event.y>=0 && i.event.x<canvas.Width && i.event.y<canvas.Height;
        }
        if (i.result.warp_pointer) i.view.pointer={i.result.pointer_x,i.result.pointer_y};
        tooltip_input(i.event);
        if (before!=i.view.tactical.TacticalPos) ++i.view.camera_generation;
        if(i.view.world)input_map_world(*i.view.world,i.event,i.result);
        ++i.view.ui_generation;
    },&input);
}
namespace {
struct AdvanceView {
 MapViewHandle& view;
 const MapDrawingContext* drawing;
 MapDrawStatistics* stats;
 DrawingStatus result=DrawingStatus::skipped;
 bool rendered=false;
};
bool advance_view(AdvanceView& a,double seconds) noexcept {
 auto& view=a.view;
 if(active!=&view||view.operation_depth||!view.terrain_loaded)return false;
 GameLoopContext loop{view.loop,view.scenario.unknown_62C!=0,&a,
  [](void* p){auto& a=*static_cast<AdvanceView*>(p);return with_map_view(a.view,[](void* q){
   auto& v=static_cast<AdvanceView*>(q)->view;
   auto before=v.tactical.TacticalPos;
   MouseClass::Instance.UpdateInput(v.pointer,v.pointer_inside);
   const auto* previous_tip=v.tooltips.CurrentToolTip;
   tooltip_poll_timer();
   if(previous_tip!=v.tooltips.CurrentToolTip)++v.ui_generation;
   if(HouseClass::CurrentPlayer) {
    const int key=0;
    PowerClass::Instance.PowerClass::Update(key,v.pointer);
    if(PowerClass::Instance.PowerNeedRedraw)++v.ui_generation;
   }
   if(RadarClass::Instance.unknown_bool_14D9||RadarClass::Instance.unknown_points_125C.Count)++v.ui_generation;
   if(before!=v.tactical.TacticalPos){++v.camera_generation;++v.ui_generation;}
  },p);},
  [](void* p){auto& a=*static_cast<AdvanceView*>(p);if(!a.drawing) {
   // Headless update retains the same presentation phase and pause behavior.
   return with_map_view(a.view,[](void* q){
    auto& v=static_cast<AdvanceView*>(q)->view;
    if(Unsorted::ArmageddonMode)return;
    RadarClass::Instance.AdvanceRadarAnimation();
    if(RadarEventClass::UpdateAll())++v.ui_generation;
    bool changed=false;
    if(RadarClass::Instance.unknown_123C)RadarClass::Instance.UpdateTerrainRadar(changed);
    RadarEventClass::RemoveFinished();
    if(changed)++v.ui_generation;
   },p);
  }
   a.result=draw_game_view(a.view,*a.drawing,*a.stats);
   a.rendered=a.result==DrawingStatus::drawn||a.result==DrawingStatus::skipped;return a.rendered;},
  [](void* p){auto& a=*static_cast<AdvanceView*>(p);return with_map_view(a.view,[](void* q){
   auto& v=static_cast<AdvanceView*>(q)->view;
   if(v.world)update_map_world(*v.world);
   if(HouseClass::CurrentPlayer&&TabClass::Instance.TabData.LastValue!=HouseClass::CurrentPlayer->Available_Money())++v.ui_generation;
   if(RadarEventClass::Array.Count||SidebarClass::Instance.SidebarNeedsRedraw||SidebarClass::Instance.Tabs[SidebarClass::Instance.ActiveTabIndex].NeedsRedraw)++v.ui_generation;
  },p);}};
 loop.scenario=&view.scenario;
 return advance_game_loop(loop,seconds);
}
}
bool update_game_view(MapViewHandle& view,double seconds) noexcept {
 AdvanceView a{view,nullptr,nullptr};return advance_view(a,seconds);
}
DrawingStatus advance_game_view(MapViewHandle& view,double seconds,const MapDrawingContext& drawing,
 MapDrawStatistics& stats,bool& rendered) noexcept {
 stats={};rendered=false;AdvanceView a{view,&drawing,&stats};
 if(!advance_view(a,seconds))return a.result==DrawingStatus::drawn||a.result==DrawingStatus::skipped?DrawingStatus::backend_failure:a.result;
 rendered=a.rendered;return a.result;
}
bool get_game_view_state(const MapViewHandle& view,GameViewState& output) noexcept {
    if (active!=&view || view.operation_depth) return false;
    output={view.ui_generation,view.scenario.unknown_62C!=0,RadarClass::Instance.IsAvailableNow,
        GadgetClass::StuckOn!=nullptr || MouseClass::Instance.unknown_byte_554A!=0,SidebarClass::Instance.ActiveTabIndex,TabClass::Instance.ThumbActive};
    output.current_frame=Unsorted::CurrentFrame;output.game_speed=GameOptionsClass::Instance.GameSpeed;
    output.logic_iterations=view.loop.iterations;output.elapsed_wall_seconds=view.loop.elapsed_seconds;output.focused=view.loop.focused;
    return true;
}
const char* map_view_error(const MapViewHandle& view) noexcept {
    return active == &view ? view.error : "Invalid map view";
}
}
