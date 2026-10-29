/*
 * Scenario world INI ordering adapts EA REDALERT/SCENARIO.CPP::Read_Scenario_INI,
 * revision f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic
 * Arts Inc.; GPL-3.0-or-later, third_party/ea/LICENSE.TXT. YR 686B20..687CCD
 * supplies the two campaign passes, missionmd metadata and dynamic modules.
 * Clear-world uses shared code; starting-unit creation remains a legacy dependency.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/GScreenClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/AbstractClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SmudgeClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SessionClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/CampaignClass.h"
#include "scenario_runtime.hpp"
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {
// Type-list phase of 0x00686B20 (0x0068797A..0x006879E8).
void read_type_definitions(CCINIClass& ai, CCINIClass& map) {
    // The global (AI) list precedes the scenario's own list; scope 1 marks it.
    TeamTypeClass::LoadFromINIList(&ai, 1);
    TeamTypeClass::LoadFromINIList(&map, 0);
    ScriptTypeClass::LoadFromINIList(&ai, 1);
    ScriptTypeClass::LoadFromINIList(&map, 0);
    TaskForceClass::LoadFromINIList(&ai, 1);
    TaskForceClass::LoadFromINIList(&map, 0);
    TriggerTypeClass::LoadFromINIList(&map);
    TagTypeClass::LoadFromINIList(&map);
    AITriggerTypeClass::LoadFromINIList(&ai, 1);
    AITriggerTypeClass::LoadFromINIList(&map, 0);
    TeamTypeClass::ProcessAllTaskforces();
}
int add(int value, unsigned amount) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) + amount); }
template <class T>
bool item(DynamicVectorClass<T*>* array, int index, T*& result) {
    if (!array || !array->Items || index < 0 || index >= array->Count || !array->Items[index]) return false;
    result = array->Items[index];
    return true;
}
struct InitializationDepth {
    int& value;
    bool active = true;
    explicit InitializationDepth(int& value) : value(value) { value = add(value, 1); }
    void finish() {
        if (active) {
            value = add(value, 0xffffffffu);
            active = false;
        }
    }
    ~InitializationDepth() { finish(); }
};
template <size_t N>
void copy_text(wchar_t (&out)[N], const wchar_t* source) {
    // Store UTF-16 units on wider hosts, as ReadUnicodeString does. Limits
    // therefore remain target code-unit limits, including a cut surrogate.
    size_t written = 0;
    while (source && *source && written < N) {
        unsigned unit = static_cast<unsigned>(*source++);
        if constexpr (sizeof(wchar_t) > 2) {
            if (unit > 0xffff && unit <= 0x10ffff) {
                unit -= 0x10000;
                out[written++] = static_cast<wchar_t>(0xd800 + (unit >> 10));
                if (written < N) out[written++] = static_cast<wchar_t>(0xdc00 + (unit & 0x3ff));
                continue;
            }
        }
        out[written++] = static_cast<wchar_t>(unit & 0xffff);
    }
    std::fill(out + written, out + N, wchar_t{});
    out[N - 1] = 0;
}
void mission_metadata(CCINIClass& ini) {
    auto& s = *ScenarioClass::Instance;
    s.Name[0] = 0;
    if (ini.Exists(s.FileName, "Name")) ini.ReadUnicodeString(s.FileName, "Name", L"", s.Name);
    s.Briefing[0] = 0;
    if (ini.Exists(s.FileName, "Briefing")) {
        ini.ReadString(s.FileName, "Briefing", "", s.BriefingCSF);
        if (s.BriefingCSF[0]) copy_text(s.Briefing, game::scenario_text(s.BriefingCSF, 3167));
    }
    s.UIName[0] = 0;
    if (ini.Exists(s.FileName, "UIName")) {
        ini.ReadString(s.FileName, "UIName", "", s.UIName);
        if (s.UIName[0]) {
            copy_text(s.Name, game::scenario_text(s.UIName, 3179));
            char label[sizeof(s.UIName) + 4];
            std::snprintf(label, sizeof(label), "%sSav", s.UIName);
            copy_text(s.UINameLoaded, game::scenario_text(label, 3183));
            if (!std::wcsncmp(s.UINameLoaded, L"MISSING:", 8)) copy_text(s.UINameLoaded, s.Name);
        }
    }
    const auto string = [&](const char* key, char* output, size_t capacity) {
        output[0] = 0;
        if (ini.Exists(s.FileName, key)) ini.ReadString(s.FileName, key, "", output, capacity);
    };
    string("LSLoadMessage", s.LSLoadMessage, sizeof(s.LSLoadMessage));
    string("LSLoadBriefing", s.LSBrief, sizeof(s.LSBrief));
#define LOCATION(field)                                                                                                \
    if (ini.Exists(s.FileName, #field)) ini.GetInteger(s.FileName, #field, s.field)
    LOCATION(LS640BriefLocX);
    LOCATION(LS640BriefLocY);
    LOCATION(LS800BriefLocX);
    LOCATION(LS800BriefLocY);
#undef LOCATION
    string("LS640BkgdName", s.LS640BkgdName, sizeof(s.LS640BkgdName));
    string("LS800BkgdName", s.LS800BkgdName, sizeof(s.LS800BkgdName));
    string("LS800BkgdPal", s.LS800BkgdPal, sizeof(s.LS800BkgdPal));
}
void scenario_overlay_name(const char* filename, char* output) {
    const char* begin = filename;
    for (const char* p = filename; *p; ++p)
        if (*p == '/' || *p == '\\' || *p == ':') begin = p + 1;
    const char* end = std::strrchr(begin, '.');
    if (!end) end = begin + std::strlen(begin);
    const size_t count = static_cast<size_t>(end - begin);
    std::memcpy(output, begin, count);
    std::memcpy(output + count, ".INI", 5);
}
RectangleStruct rectangle(CCINIClass& ini, const char* key, const RectangleStruct& fallback) {
    int values[4], defaults[4]{fallback.X, fallback.Y, fallback.Width, fallback.Height};
    ini.Read4Integers(values, "Map", key, defaults);
    return {values[0], values[1], values[2], values[3]};
}
bool complete(const game::ScenarioRuntimeServices& r) {
    const auto* s = r.initialize;
    return ScenarioClass::Instance && s && CCINIClass::INI_Rules && RulesClass::Instance && s->tactical_rect &&
           s->theater && s->side && s->read_objects && s->step && r.world && r.world->message_rect;
}
// The multiplayer branch also prepares the session's game mode and the
// starting units through the original multiplayer services.
bool complete_multiplayer(const game::ScenarioRuntimeServices& r) {
    const auto* s = r.initialize;
    return s->set_map_size && s->prepare_mode && s->start_mode && s->start_special_mode && s->mode_ready &&
           r.world->legacy_starting_units;
}
}

bool YRPP_FASTCALL ScenarioClass::InitializeWorldINI(CCINIClass* ini, bool skip_units) {
    const auto& r = game::scenario_runtime();
    if (!ini || !complete(r) || !std::memchr(Instance->FileName, 0, sizeof(Instance->FileName))) return false;
    const auto& s = *r.initialize;
    auto& map = MapClass::Instance;
    auto& session = SessionClass::Instance;
    const auto mode = [&] { return static_cast<int>(SessionClass::Instance.GameMode); };
    // Loading-screen progress and the event pump belong to the host; a host
    // without a loading screen leaves them unset.
    const auto pump = [&] {
        if (r.ini && r.ini->pump_events) r.ini->pump_events(r.ini->context);
    };
    const auto progress = [&](int value) {
        if (r.ini && r.ini->progress) r.ini->progress(r.ini->context, value);
    };
    const auto read = [&](game::ScenarioObjectReader reader, CCINIClass* source) {
        s.read_objects(s.context, reader, source);
    };
    const auto step = [&](game::ScenarioInitializationStep value) { s.step(s.context, value); };
    // 0x0050C170 on the player named by [Basic]; an unknown name selects side 0.
    const auto player_side = [&] {
        char player[32];
        ini->ReadString("Basic", "Player", "Americans", player);
        const int index = HouseClass::FindIndexByName(player);
        return index < 0 || index >= HouseClass::Array.Count || !HouseClass::Array[index]->Type
                   ? 0
                   : HouseClass::Array[index]->Type->SideIndex;
    };
    if (mode() != 0 && !complete_multiplayer(r)) return false;
    InitializationDepth depth(Unsorted::ScenarioInit);
    const auto fail = [&] {
        depth.finish();
        return false;
    };
    bool first_campaign_pass = mode() == 0;
    for (;;) {
        if (!first_campaign_pass) ScenarioClass::ClearWorld();
        const bool campaign = mode() == 0;
        Instance->Difficulty1 = campaign ? GameOptionsClass::Instance.Difficulty : session.Config.AIDifficulty;
        Instance->Difficulty2 = 2u - Instance->Difficulty1;
        Instance->SpecialFlags.FogOfWar = !campaign && session.Config.FogOfWar;
        ScenarioClass::GlobalSpecialFlags.FogOfWar = !campaign && session.Config.FogOfWar;
        Instance->InitTime = ini->ReadInteger("Basic", "InitTime", 10000);
        const bool official = ini->ReadBool("Basic", "Official", false);
        CCINIClass auxiliary;
        CCFileClass auxiliary_file;
        auto* file = &auxiliary_file;
        if (mode() == 0) {
            if (!first_campaign_pass) {
                char filename[sizeof(Instance->FileName) + 5];
                scenario_overlay_name(Instance->FileName, filename);
                file->SetFileName(filename);
                if (file->Exists(false)) {
                    auxiliary.ReadCCFile(file, false, false);
                    RulesClass::Instance->Read_File(&auxiliary);
                }
                file->Close();
            }
            file->SetFileName("MISSIONMD.INI");
            if (file->Exists(false)) {
                auxiliary.ReadCCFile(file, false, false);
                mission_metadata(auxiliary);
            }
        }
        std::fill_n(Instance->HouseIndices, MaxStartingPositions, -1);
        if (mode() != 0) {
            if (!Instance->ReadWaypoints(*ini)) return fail();
            if (Instance->IsRandom) std::copy_n(Instance->HouseHomeCells, 8, Instance->Waypoints);
            RulesClass::Read_Countries(CCINIClass::INI_Rules);
            RulesClass::Instance->Read_General(CCINIClass::INI_Rules);
            for (int i = 0; i < HouseTypeClass::Array.Count; ++i) {
                HouseTypeClass* country = nullptr;
                if (!item(&HouseTypeClass::Array, i, country)) return fail();
                country->LoadFromINI(CCINIClass::INI_Rules);
            }
            AssignHouses();
            s.set_map_size(s.context, rectangle(*ini, "Size", {1, 1, 50, 50}), true, false, true);
            if (!Instance->IsRandom) Instance->ReadStartPoints(*ini);
            RadarClass::Instance.SetVisibleRect(rectangle(*ini, "LocalSize", map.VisibleRect));
            auto* game_mode = session.MPGameMode;
            if (!game_mode) return fail();
            s.prepare_mode(s.context, game_mode, official);
            if (session.unknown_0C == 2)
                s.start_special_mode(s.context, official);
            else
                s.start_mode(s.context, session.MPGameMode, official);
        }
        // A host without a loading screen supplies no manager.
        LoadProgressManager* manager = nullptr;
        if (r.world->screen && r.world->screen->get_manager && s.draw_load_screen) {
            if (!r.world->screen->get_manager(r.world->screen->context, manager) || !manager) return fail();
            s.draw_load_screen(s.context, manager);
        }
        progress(3);
        if (first_campaign_pass) {
            first_campaign_pass = false;
            continue;
        }
        // 0x006875F3: delete the previous object, then allocate and construct.
        if (TacticalClass::Instance) delete TacticalClass::Instance;
        TacticalClass::Instance = new TacticalClass();
        s.tactical_rect(s.context, TacticalClass::Instance, *r.world->message_rect);
        Instance->Theater = static_cast<TheaterType>(ini->ReadTheater("Map", "Theater", 0));
        s.theater(s.context, static_cast<int>(Instance->Theater));
        progress(30);
        RulesClass::Read_AdvancedCommandBar(&CCINIClass::INI_UIMD, mode() != 0 && mode() != 5);
        progress(31);
        RulesClass::Instance->Init(CCINIClass::INI_Rules);
        progress(35);
        pump();
        if (!Instance->ReadGlobalVariables(*CCINIClass::INI_Rules)) return fail();
        pump();
        RulesClass::Instance->Read_File(ini);
        progress(45);
        ScenarioClass::ScenarioCRC = 0;
        int side = 0;
        if (mode() == 0) {
            // Keep the campaign lookup even though House selection below
            // replaces the result, as the original registry access requires it.
            if (Instance->CampaignIndex != -1) {
                CampaignClass* campaign = nullptr;
                if (!item(&CampaignClass::Array, Instance->CampaignIndex, campaign)) return fail();
            }
            HouseClass::LoadFromINIList(ini);
            side = player_side();
        } else {
            NodeNameType* player = nullptr;
            HouseTypeClass* country = nullptr;
            if (!item(&NodeNameType::Array, 0, player) ||
                !item(&HouseTypeClass::Array, player->Country == -3 ? 0 : player->Country, country))
                return fail();
            side = country->SideIndex;
        }
        Instance->PlayerSideIndex = side;
        if (!s.side(s.context, side)) return fail();
        progress(50);
        if (!Instance->ReadINI(*ini)) return fail();
        progress(58);
        if (mode() == 0) {
            side = player_side();
            if (!s.side(s.context, side)) return fail();
            Instance->PlayerSideIndex = side;
        } else {
            Instance->PlayerSideIndex = side;
            if (session.MPGameMode) s.mode_ready(s.context, session.MPGameMode);
        }
        if (mode() != 0 && !session.Config.BridgeDestruction)
            ScenarioClass::GlobalSpecialFlags.DestroyableBridges = false;
        pump();
        read_type_definitions(CCINIClass::INI_AI, *ini);
        progress(60);
        read(game::ScenarioObjectReader::map, ini);
        pump();
        TubeClass::ReadINI(ini);
        BuildingTypeClass::InitToTiles();
        GScreenClass::Instance.MarkNeedsRedraw(2);
        progress(70);
        pump();
        OverlayClass::ReadINI(ini);
        pump();
        map.CellIteratorReset();
        while (auto* cell = map.CellIteratorNext())
            cell->RecalcAttributes(-1);
        OverlayClass::RebuildVeins();
        TerrainClass::ReadINI(ini);
        pump();
        // 0x74DE90 (VeinholeMonsterClass::InitVeinGrowthData) is not called:
        // native scenarios never create veinhole monsters, and without one it
        // only clears overlay 126, which no stock map places.
        TiberiumClass::InitGrowthLogic();
        TiberiumClass::InitSpreadLogic();
        progress(72);
        RadarClass::Instance.ComputeRadarImage();
        UnitClass::ReadINI(ini);
        pump();
        progress(74);
        AircraftClass::ReadINI(ini);
        pump();
        InfantryClass::ReadINI(ini);
        pump();
        progress(76);
        ScenarioClass::BuildingReadFlag = false;
        BuildingClass::ReadINI(ini);
        pump();
        progress(78);
        ScenarioClass::BuildingReadFlag = true;
        pump();
        SmudgeClass::ReadINI(ini);
        pump();
        if (mode() == 5 && ScenarioClass::CustomAI) {
            file->Close();
            file->SetFileName("TMCJ4F.INI");
            if (file->Exists(false)) {
                auxiliary.ReadCCFile(file, false, false);
                RulesClass::Instance->Read_File(&auxiliary);
            }
        }
        progress(82);
        pump();
        map.TotalValue = map.Overpass(false);
        progress(86);
        pump();
        BeaconManagerClass::Instance.LoadArt();
        progress(90);
        pump();
        if (mode() != 0 && !skip_units)
            r.world->legacy_starting_units(r.world->context, ini->ReadBool("Basic", "Official", false));
        pump();
        progress(96);
        pump();
        AbstractClass::RemoveAllInactive();
        if (mode() != 0) Instance->SpecialFlags = ScenarioClass::GlobalSpecialFlags;
        depth.finish();
        {
            struct RestoreDepth {
                int& value;
                int after;
                ~RestoreDepth() { value = after; }
            } restore{depth.value, depth.value};
            depth.value = 0;
            step(game::ScenarioInitializationStep::operational_buildings);
        }
        progress(98);
        pump();
        // 0x660C80 preserves borrowed Items and IsInitialized. The native
        // vector's explicit Clear also resets borrowed Items, so reproduce
        // this original slot cleanup without changing generic vector policy.
        auto& radar_cells = RadarClass::Instance.unknown_cells_1124;
        radar_cells.Count = 0;
        if (radar_cells.Items && radar_cells.IsAllocated) {
            DLLDeleteArray(radar_cells.Items, static_cast<size_t>(radar_cells.Capacity));
            radar_cells.Items = nullptr;
        }
        radar_cells.IsAllocated = false;
        radar_cells.Capacity = 0;
        if (Instance->SpecialFlags.FogOfWar) step(game::ScenarioInitializationStep::fog);
        // 0x00660B00 is a copy of RadarEventClass::Clear (0x0065FD50).
        RadarEventClass::Clear();
        step(game::ScenarioInitializationStep::redraw_map);
        // Native weapon contents finish after TechnoType INI readers. Resolve
        // the original 0x715793 flag before FinishWorld queries buildability.
        for (auto* type : TechnoTypeClass::Array)
            type->HasPrimary = type->Weapon[0].WeaponType && type->Weapon[0].WeaponType->Damage > 0;
        return true;
    }
}
