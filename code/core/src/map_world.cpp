#include "tactical_spatial.hpp"
#include "visibility_grid.hpp"
#include "yrpp/FactoryClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/MouseClass.h"
#include "map_world_internal.hpp"
#include "runtime_diagnostics.hpp"
#include "yrpp/AITriggerTypeClass.h"
#include "map_configuration.hpp"
#include "map_view.hpp"
#include "map_runtime.hpp"
#include "type_resources.hpp"
#include "audio_session.hpp"
#include "yrpp/RulesClassReaders.hpp"
#include "api/rules_runtime.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/RadSiteClass.h"
#include "yrpp/BombListClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/Powerups.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/LineTrail.h"
#include "yrpp/EBolt.h"
#include "yrpp/LaserDrawClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/SideClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/Surface.h"
#include "yrpp/GadgetClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SessionClass.h"
#include "yrpp/EventClass.h"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace game {
namespace {
// The single live world: native hosts run one scenario at a time.
MapWorld* active_world = nullptr;
std::atomic<std::uint64_t> next_world{1};
void read_optional(CCINIClass& ini, const char* name, const char* fallback = nullptr) {
    CCFileClass f(name);
    if (!f.Exists() && fallback) f.SetFileName(fallback);
    if (f.Exists() && ini.ReadCCFile(&f, false, false) <= 0) throw std::runtime_error("Cannot read type INI");
}
bool palette_file(const char* name, BytePalette& out) {
    CCFileClass f(name);
    if (!f.Exists() || f.GetFileSize() < 768 || f.ReadBytes(&out, 768) != 768) return false;
    for (auto& c : out.Entries) {
        c.R = BYTE(c.R << 2);
        c.G = BYTE(c.G << 2);
        c.B = BYTE(c.B << 2);
    }
    return true;
}

}
// The theater palettes of 0x005349C0 that this host keeps per world.
void load_world_palettes(MapWorld& world) {
    auto& w = *world.impl;
    w.presentation.voxel_palette_loaded = load_voxel_palette(w.presentation.voxel_palette);
    char name[64];
    auto& t = Theater::GetTheater(w.view.scenario.Theater);
    std::snprintf(name, sizeof(name), "UNIT%s.PAL", t.Extension);
    if (!palette_file(name, w.presentation.unit_palette)) {
        w.presentation.unit_palette = FileSystem::TEMPERAT_PAL;
        ++w.missing_palettes;
    }
    w.presentation.selection_palette_loaded = palette_file("PALETTE.PAL", w.presentation.selection_palette);
    if (!w.presentation.selection_palette_loaded) {
        w.presentation.selection_palette = w.presentation.unit_palette;
        ++w.missing_palettes;
    }
    if (!palette_file("ANIM.PAL", w.presentation.anim_palette)) {
        w.presentation.anim_palette = w.presentation.unit_palette;
        ++w.missing_palettes;
    }
}
ColorStruct world_house_hsv(const HouseClass& owner) {
    // The house color is its scheme: HouseClass::LoadFromINI (0x00500B40) resolves
    // Color= to ColorSchemeIndex, and BaseColor is the HSV the scheme was built from.
    if (auto* scheme = ColorScheme::Array.GetItemOrDefault(owner.ColorSchemeIndex)) return scheme->BaseColor;
    return ColorStruct{0, 0, 128};
}
MapWorld::Impl::Impl(MapViewHandle& v) : view(v), world_id(next_world.fetch_add(1)) {
    // First indices of this world's objects in the original registries.
    building_types = BuildingTypeClass::Array.Count;
    terrain_types = TerrainTypeClass::Array.Count;
    buildings = BuildingClass::Array.Count;
    terrains = TerrainClass::Array.Count;
    animations = AnimClass::Array.Count;
    infantry = InfantryClass::Array.Count;
    units = UnitClass::Array.Count;
    aircraft = AircraftClass::Array.Count;
    bullets = BulletClass::Array.Count;
    line_trails = LineTrail::Array.Count;
    particle_systems = ParticleSystemClass::Array.Count;
    electric_bolts = EBolt::Array.Count;
    lasers = LaserDrawClass::Array.Count;
}
MapWorld::MapWorld(MapViewHandle& v) : impl(std::make_unique<Impl>(v)) {
    active_world = this;
    TechnoClass::ActionLineTimer.Start(0);
    type_services.context = this;
    type_services.art = &art_ini;
    type_services.theater = &v.scenario.Theater;
    type_services.audio_unavailable = !native_audio_ready();
    type_services.combat_unavailable = false;
    type_services.terrain_foundation = native_foundation;
    type_services.arctic_image_name = native_arctic_name;
}
MapWorld::~MapWorld() {
    stop_tactical_spatial();
    // ScenarioClass::ClearWorld has already retired the scenario's objects and
    // types; only this host's bindings remain.
    ColorScheme::ReleasePaletteCache();
    TacticalClass::Instance->EndRubberBand();
    TacticalClass::Instance->SelectableCount = 0;
    // Pending events belong to this scenario and never reach the next one.
    EventClass::OutList.Init();
    EventClass::DoList.Init();
    EventClass::MegaMissionList.Init();
    Game::IsActive = false;
    Unsorted::ScenarioStarted = false;
    if (active_world == this) active_world = nullptr;
}
bool native_arctic_name(void*, const char* base, char* out, std::size_t size) noexcept {
    if (!base || !out || !size) return false;
    int n = std::snprintf(out, size, "%sA", base);
    return n >= 0 && std::size_t(n) < size;
}
bool with_map_world(MapWorld* world, void (*op)(void*), void* arg) noexcept {
    if (!op) return false;
    try {
        if (!world) {
            op(arg);
            return true;
        }
        auto runtime = map_runtime();
        // CoordsToClient (0x006D2140) reads the active Tactical viewport. Bind it
        // for every native world operation, including LineTrail::Draw; the default
        // host runtime deliberately has no viewport and leaves projection output
        // untouched when it is unavailable.
        runtime.view_bounds = &TacticalClass::ViewBounds;
        runtime.drawing_bounds = DSurface::ViewBounds.Width > 0 ? &DSurface::ViewBounds : &TacticalClass::ViewBounds;
        runtime.has_window = []() noexcept { return true; };
        runtime.action_line_palette = &world->impl->presentation.selection_palette;
        struct Call {
            MapWorld& world;
            void (*operation)(void*);
            void* argument;
            bool complete = false;
        } call{*world, op, arg};
        const bool invoked = with_map_runtime(
            runtime,
            [](void* p) {
                auto& call = *static_cast<Call*>(p);
                call.complete = with_type_resources(call.world.type_services, call.operation, call.argument) ==
                                TypeResourceStatus::complete;
            },
            &call);
        return invoked && call.complete;
    } catch (...) {
        return false;
    }
}
void map_object_changed() noexcept {
    if (active_world) ++active_world->presentation_revision;
}
int active_map_player_power_bonus(const HouseClass* house) noexcept {
    return active_world && house == HouseClass::CurrentPlayer ? active_world->impl->player_power_bonus : 0;
}
void changed_world_cell(MapWorld& world, CellClass& cell) {
    auto& w = *world.impl;
    const int i = MapClass::GetCellIndex(cell.MapCoords);
    if (i < 0 || std::size_t(i) >= w.resource_membership.size()) return;
    const int resource = cell.GetContainedTiberiumIndex();
    if (resource >= 0 && !w.resource_membership[i]) {
        w.resource_cells.push_back(&cell);
        w.resource_membership[i] = 1;
    }
}
void map_resource_changed(CellClass& cell) noexcept {
    if (!active_world) return;
    try {
        changed_world_cell(*active_world, cell);
    } catch (...) {
        ++active_world->impl->unknown_records;
    }
    ++active_world->resource_revision;
    ++active_world->presentation_revision;
    RadarClass::Instance.MarkTerrainCellDirty(cell.MapCoords);
}
static_assert(SkirmishSlotCount == MaxPlayerSlots && SkirmishColorCount <= MaxHouseBits);
// Native player colors 8..29 in HSV, after the original eight (0x83ED14): eleven
// hues between the originals, each as a light and a deep shade.
const ColorStruct extra_player_colors[SkirmishColorCount - 8]{
    {12, 140, 255}, {58, 140, 255}, {81, 140, 255}, {104, 140, 255}, {127, 140, 255}, {150, 140, 255},
    {173, 140, 255}, {196, 140, 255}, {219, 140, 255}, {242, 140, 255}, {35, 140, 255},
    {12, 255, 170}, {58, 255, 170}, {81, 255, 170}, {104, 255, 170}, {127, 255, 170}, {150, 255, 170},
    {173, 255, 170}, {196, 255, 170}, {219, 255, 170}, {242, 255, 170}, {35, 255, 170}};
// AssignHouses maps player colors 0..8 through the original table and uses
// larger values as scheme indices. Replace native colors with their schemes;
// those follow the rules colors, so they never read as a color slot again.
static void resolve_extra_player_colors(MapWorld& world) {
    int schemes[SkirmishColorCount - 8];
    for (int i = 0; i < SkirmishColorCount - 8; ++i) {
        char id[32];
        std::snprintf(id, sizeof(id), "OpenRA2Player%d", i + 8);
        schemes[i] = ColorScheme::FindByName(id, extra_player_colors[i], world.impl->presentation.unit_palette,
                                             FileSystem::TEMPERAT_PAL, 53);
        if (schemes[i] < SkirmishColorCount) throw std::runtime_error("Could not prepare extra player colors");
    }
    const auto resolve = [&](int& color) {
        if (color >= 8 && color < SkirmishColorCount) color = schemes[color - 8];
    };
    auto& slots = SessionClass::Instance.Config.AISlots;
    for (int i = 0; i < MaxPlayerSlots; ++i)
        if (slots.Countries[i] != -1) resolve(slots.Colors[i]);
    for (auto* player : NodeNameType::Array) {
        resolve(player->Color);
        resolve(player->InitialColor);
    }
}
// Frontend adaptation only: the live game still owns the original Session,
// NodeName, House and MPBattle objects. Resolve random menu choices before
// AssignHouses, as the original dialog does through 0x69B8C0.
static void prepare_skirmish(MapWorld& world, const SkirmishOptions& setup) {
    auto& view = world.impl->view;
    if (NodeNameType::Array.Count) throw std::runtime_error("Skirmish already has external player records");
    int starts = 0;
    for (; starts < MaxPlayerSlots; ++starts) {
        char key[4];
        std::snprintf(key, sizeof(key), "%d", starts);
        if (world.map_ini.ReadInteger("Waypoints", key, -1) < 0) break;
    }
    auto& rules = world.base_rules_ini;
    std::vector<int> countries;
    for (int i = 0; i < rules.GetKeyCount("Countries"); ++i) {
        char id[256]{};
        rules.ReadString("Countries", rules.GetKeyName("Countries", i), "", id);
        if (id[0] && rules.ReadBool(id, "Multiplay", false) && !rules.ReadBool(id, "MultiplayPassive", false))
            countries.push_back(i);
    }
    if (countries.empty()) throw std::runtime_error("No playable multiplayer countries");
    int count = 0;
    unsigned used_colors = 0;
    for (const auto& slot : setup.players) {
        if (slot.kind == -1) continue;
        ++count;
        if (slot.start >= starts) throw std::runtime_error("Starting position is not present on this map");
        if (slot.country != -2 && std::find(countries.begin(), countries.end(), slot.country) == countries.end())
            throw std::runtime_error("Selected country is not playable");
        if (slot.color >= 0) used_colors |= 1u << slot.color;
    }
    if (count > starts) throw std::runtime_error("Map has fewer starting positions than players");
    auto& config = SessionClass::Instance.Config;
    config = {};
    config.ScenarioIndex = -1;
    config.Bases = true;
    config.Money = setup.money;
    config.UnitCount = setup.unit_count;
    config.GameSpeed = setup.game_speed;
    config.AIDifficulty = 1;
    config.AIPlayers = count - 1;
    config.ShortGame = setup.short_game;
    config.SWAllowed = setup.super_weapons;
    config.BuildOffAlly = setup.build_off_ally;
    config.MCVRedeploy = setup.mcv_redeploy;
    config.Crates = setup.crates;
    config.BridgeDestruction = setup.bridge_destruction;
    config.AlliesAllowed = true;
    for (int i = 0; i < MaxPlayerSlots; ++i) {
        config.AISlots.Countries[i] = -1;
        config.AISlots.Colors[i] = -1;
        config.AISlots.Starts[i] = -2;
        config.AISlots.Allies[i] = -1;
        config.AISlots.Difficulties[i] = 1;
    }
    // Games that fit the original eight colors draw random colors from those.
    const int palette = count <= 8 ? 8 : SkirmishColorCount;
    Randomizer random(setup.seed);
    for (int i = 0; i < MaxPlayerSlots; ++i) {
        const auto& slot = setup.players[i];
        if (slot.kind == -1) continue;
        const int country = slot.country == -2 ? countries[random.RandomRanged(0, int(countries.size()) - 1)] : slot.country;
        int color = slot.color;
        if (color == -2) {
            do { color = random.RandomRanged(0, palette - 1); } while (used_colors & (1u << color));
            used_colors |= 1u << color;
        }
        if (!i) {
            auto& player = view.local_player;
            player = {};
            std::wcsncpy(player.Name, L"Commander", 19);
            player.SetCountry(country);
            player.Color = color;
            player.InitialColor = -1;
            player.StartPoint = player.InitialStartPoint = slot.start;
            player.Team = player.InitialTeam = slot.team;
            if (!NodeNameType::Array.AddItem(&player)) throw std::bad_alloc();
        } else {
            config.AISlots.Countries[i] = country;
            config.AISlots.Colors[i] = color;
            config.AISlots.Starts[i] = slot.start;
            config.AISlots.Allies[i] = slot.team;
            config.AISlots.Difficulties[i] = slot.kind;
        }
    }
    // Level 9 is the host's uncapped equivalent of original delay zero.
    if (!set_game_loop_speed(view.loop, setup.game_speed ? 7 - setup.game_speed : 9))
        throw std::runtime_error("Could not apply skirmish game speed");
}
bool load_map_world(MapWorld& world, char* error, std::size_t error_size, const SkirmishOptions* skirmish) {
    if (!error || !error_size) return false;
    error[0] = 0;
    auto& w = *world.impl;
    try {
        start_tactical_spatial();
        // Startup (0x0052BA60 and Init_Game) reads these once; a native view reads
        // them with each map, from the same mounted resources.
        read_optional(world.base_rules_ini, "RULESMD.INI", "RULES.INI");
        read_optional(world.art_ini, "ARTMD.INI", "ART.INI");
        CCINIClass::INI_AI.Clear();
        read_optional(CCINIClass::INI_AI, "AIMD.INI", "AI.INI");
        CCINIClass::INI_UIMD.Clear();
        read_optional(CCINIClass::INI_UIMD, "UIMD.INI");
        CCINIClass::INI_Rules = &world.base_rules_ini;
        if (world.art_ini.GetSection("Movies") && !RulesClass::Read_Movies(&world.art_ini))
            throw std::runtime_error("Movie name configuration parsing failed");
        auto& session = SessionClass::Instance;
        if (session.GameMode == GameMode::Campaign) {
            // Init Rules copies this option before scenario overlays and the
            // first House/Trigger update; a fresh native Session is all zero.
            RulesClass::Instance->Read_MultiplayerDialogSettings(&world.base_rules_ini);
            initialize_campaign_superweapon_option(*RulesClass::Instance, session);
        }
        if (skirmish) prepare_skirmish(world, *skirmish);
        if (session.GameMode != GameMode::Campaign) {
            auto& view = world.impl->view;
            if (!NodeNameType::Array.Count) {
                const char* settings = "MultiplayerDialogSettings";
                auto& config = session.Config;
                config.SWAllowed = world.base_rules_ini.ReadBool(settings, "SuperWeaponsAllowed", true);
                config.MCVRedeploy = world.base_rules_ini.ReadBool(settings, "MCVRedeploys", true);
                config.ScenarioIndex = -1;
                config.Bases = world.base_rules_ini.ReadBool(settings, "Bases", true);
                config.Money = world.base_rules_ini.ReadInteger(settings, "Money", 10000);
                config.UnitCount = world.base_rules_ini.ReadInteger(settings, "UnitCount", 10);
                config.AIDifficulty = world.base_rules_ini.ReadInteger(settings, "AIDifficulty", 1);
                config.AIPlayers = 1;
                config.BridgeDestruction = world.base_rules_ini.ReadBool(settings, "BridgeDestruction", true);
                config.Crates = world.base_rules_ini.ReadBool(settings, "Crates", true);
                config.ShortGame = world.base_rules_ini.ReadBool(settings, "ShortGame", true);
                config.AlliesAllowed = true;
                auto& player = view.local_player;
                std::wcsncpy(player.Name, L"Player", 19);
                player.Country = player.InitialCountry = 0;
                player.Color = player.InitialColor = 0;
                player.StartPoint = player.InitialStartPoint = -2;
                player.Team = player.InitialTeam = -1;
                player.SpectatorFlag = 0;
                if (!NodeNameType::Array.AddItem(&player)) throw std::bad_alloc();
                for (int i = 0; i < MaxPlayerSlots; ++i) {
                    config.AISlots.Countries[i] = i ? -1 : 5;
                    config.AISlots.Colors[i] = i + 1;
                    config.AISlots.Starts[i] = -2;
                    config.AISlots.Allies[i] = -1;
                    config.AISlots.Difficulties[i] = config.AIDifficulty;
                }
            }
            Game::PlayerCount = NodeNameType::Array.Count + session.Config.AIPlayers;
            if (!session.MPGameMode) {
                wchar_t* title = const_cast<wchar_t*>(L"Battle");
                char* file = const_cast<char*>("MPBATTLE.INI");
                char* filter = const_cast<char*>("standard");
                view.battle_mode = std::make_unique<MPBattleClass>(&title, &title, &file, &filter, true, 0);
                session.MPGameMode = view.battle_mode.get();
            }
        }
        Game::IsActive = true;
        struct Call {
            MapWorld& world;
            char* error;
            std::size_t size;
            bool ok = false;
        } call{world, error, error_size};
        const auto status = with_type_resources(
            world.type_services,
            [](void* p) {
                auto& c = *static_cast<Call*>(p);
                // Keep the reason: the resource scope reports only a failed status.
                try {
                    with_map_rules_runtime(
                        c.world,
                        [](void* q) {
                            auto& c = *static_cast<Call*>(q);
                            // The original frontend already has colors/palettes for
                            // the preliminary multiplayer House assignment. A fresh
                            // native view must prepare that prerequisite explicitly.
                            if (SessionClass::Instance.GameMode != GameMode::Campaign && !ColorScheme::Array.Count) {
                                auto& scenario = *ScenarioClass::Instance;
                                scenario.Theater = static_cast<TheaterType>(c.world.map_ini.ReadTheater("Map", "Theater", 0));
                                if (!Theater::MountResourceMixes(scenario.Theater) ||
                                    !FileSystem::LoadTheaterPalettes(scenario.Theater))
                                    throw std::runtime_error("Could not prepare multiplayer colors");
                                load_world_palettes(c.world);
                                RulesClass::Read_Colors(CCINIClass::INI_Rules);
                            }
                            if (SessionClass::Instance.GameMode != GameMode::Campaign) resolve_extra_player_colors(c.world);
                            c.ok = ScenarioClass::StartScenario(
                                ScenarioClass::Instance->FileName,
                                c.world.impl->view.continuation_loading &&
                                    c.world.impl->view.transition.result == GameResult::victory,
                                c.world.impl->view.continuation_loading ? c.world.impl->view.next_campaign : -1);
                        },
                        p);
                } catch (const std::exception& e) {
                    std::snprintf(c.error, c.size, "%s", e.what());
                    throw;
                }
            },
            &call);
        if (status != TypeResourceStatus::complete) {
            if (error[0]) {
            } else if (last_type_resource_failure()[0])
                std::snprintf(error, error_size,
                              "Could not load map object types and art: Cannot load type %s (resource status %d)",
                              last_type_resource_failure(), static_cast<int>(status));
            else
                std::snprintf(error, error_size, "Could not load map object types and art (resource status %d)",
                              static_cast<int>(status));
            return false;
        }
        if (!call.ok) throw std::runtime_error("Scenario initialization failed");
        auto& map = MapClass::Instance;
        w.resource_membership.assign(MapClass::MaxCells, 0);
        for (int i = 0; i < map.Cells.Capacity; ++i)
            if (auto* cell = map.Cells[i]) changed_world_cell(world, *cell);
        for (int i = w.building_types; i < BuildingTypeClass::Array.Count; ++i)
            if (!BuildingTypeClass::Array[i]->Image) ++w.missing_images;
        for (int i = w.terrain_types; i < TerrainTypeClass::Array.Count; ++i)
            if (!TerrainTypeClass::Array[i]->Image) ++w.missing_images;
        w.loaded = true;
        ++world.presentation_revision;
        return true;
    } catch (const std::exception& e) {
        if (!error[0]) std::snprintf(error, error_size, "%s", e.what());
    } catch (...) {
        if (!error[0]) std::snprintf(error, error_size, "Map scenario loading failed");
    }
    return false;
}
void end_map_world_frame(MapWorld& world) {
    auto& w = *world.impl;
    if (!w.loaded) return;
    // The step has already run; the begin record only keeps the tick context current.
    runtime_log_tick(w.world_id, world.simulation_tick, false);
    const bool projectile_visuals = BulletClass::Array.Count > w.bullets || LineTrail::Array.Count > w.line_trails ||
                                    ParticleSystemClass::Array.Count > w.particle_systems ||
                                    EBolt::Array.Count > w.electric_bolts || LaserDrawClass::Array.Count > w.lasers;
    if (w.projectile_visuals || projectile_visuals) map_object_changed();
    w.projectile_visuals = projectile_visuals;
    flush_tactical_spatial();
    game::visibility_frame_end();
    ++world.simulation_tick;
    runtime_log_tick(w.world_id, world.simulation_tick, true);
    refresh_map_world_hover(world);
}
MapObjectId object_id(const MapWorld& w, const ObjectClass* o) noexcept {
    return o ? MapObjectId{w.impl->world_id, std::uint32_t(o->UniqueID)} : MapObjectId{};
}
ObjectClass* resolve_map_object(MapWorld& world, MapObjectId id) noexcept {
    if (id.world != world.impl->world_id) return nullptr;
    // A live map owns the original registries. Keep the host generation guard,
    // then resolve through the same identity index as original TargetClass.
    AbstractClass* object = nullptr;
    if (!AbstractClass::TargetIndex.TryGet(std::bit_cast<int>(id.value), object) || !object) return nullptr;
    switch (object->WhatAmI()) {
        case AbstractType::Building:
        case AbstractType::Terrain:
        case AbstractType::Infantry:
        case AbstractType::Unit:
        case AbstractType::Aircraft:
            return static_cast<ObjectClass*>(object);
        default:
            return nullptr;
    }
}
namespace {
struct WorldPointer {
    CellStruct cell{};
    CoordStruct coords{};
    ObjectClass* object = nullptr;
    BYTE fogged = 0, shrouded = 0;
};
bool resolve_world_pointer(MapWorld& world, Point2D p, WorldPointer& out) {
    auto b = DSurface::ViewBounds;
    if (b.Width <= 0 || b.Height <= 0) b = TacticalClass::ViewBounds;
    if (p.X < b.X || p.Y < b.Y || p.X >= b.X + b.Width || p.Y >= b.Y + b.Height) return false;
    p.X -= b.X;
    p.Y -= b.Y;
    rebuild_world_selectables(world);
    return DisplayClass::Instance.ProcessClickCoords(&p, &out.cell, &out.coords, &out.object, &out.fogged,
                                                     &out.shrouded);
}
}
void refresh_map_world_hover(MapWorld& world) {
    auto& w = *world.impl;
    // Pointer events and batched logic steps reach the host outside the original
    // Input phase, so the previous flag is cleared here instead of waiting for
    // TechnoClass::Update; the flag itself is set by the original path below.
    if (auto* previous = resolve_map_object(world, w.hover);
        previous && (previous->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None)
        static_cast<TechnoClass*>(previous)->IsMouseHovering = false;
    WorldPointer pointer;
    const bool picked = w.view.pointer_inside && resolve_world_pointer(world, w.view.pointer, pointer);
    // The idle branch of ScrollClass::PollMouse (0x00692F30): Display::ConvertAction
    // (0x004AAE90) marks the object under the pointer as hovered.
    auto& mouse = MouseClass::Instance;
    if (picked && HouseClass::CurrentPlayer && !Unsorted::UserInputLocked && mouse.unknown_byte_554A != 1)
        DisplayClass::Instance.DisplayClass::ConvertAction(pointer.cell, pointer.shrouded != 0, pointer.object,
                                                           mouse.DecideAction(pointer.cell, pointer.object, 1), false);
    const auto id = object_id(world, picked ? pointer.object : nullptr);
    if (id.world != w.hover.world || id.value != w.hover.value) {
        w.hover = id;
        ++world.presentation_revision;
    }
}
ObjectClass* pick_world_object(MapWorld& world, Point2D p) {
    WorldPointer pointer;
    return resolve_world_pointer(world, p, pointer) ? pointer.object : nullptr;
}

MapWorld* current_map_world() noexcept { return active_world; }
bool current_map_pointer_cell(CellStruct& cell) noexcept {
    auto* world = current_map_world();
    if (!world || !world->impl) return false;
    auto& view = world->impl->view;
    auto bounds = DSurface::ViewBounds;
    if (bounds.Width <= 0 || bounds.Height <= 0) bounds = TacticalClass::ViewBounds;
    if (bounds.Width <= 0 || bounds.Height <= 0) return false;
    cell = CellStruct::Empty;
    return TacticalClass::Instance->PickTerrainCell(view.pointer, bounds, cell);
}
}
