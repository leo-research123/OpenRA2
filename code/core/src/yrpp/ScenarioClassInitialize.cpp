/*
 * Scenario world INI ordering adapts EA REDALERT/SCENARIO.CPP::Read_Scenario_INI,
 * revision f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic
 * Arts Inc.; GPL-3.0-or-later, third_party/ea/LICENSE.TXT. YR 686B20..687CCD
 * supplies the two campaign passes, missionmd metadata and dynamic modules.
 * Clear-world uses shared code; starting-unit creation remains a legacy dependency.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "scenario_runtime.hpp"
#include "scenario_loading.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {
int add(int value, unsigned amount) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) + amount);
}
template<class T> bool item(DynamicVectorClass<T*>* array, int index, T*& result) {
    if (!array || !array->Items || index < 0 || index >= array->Count || !array->Items[index]) return false;
    result = array->Items[index]; return true;
}
struct InitializationDepth {
    int& value;
    bool active = true;
    explicit InitializationDepth(int& value) : value(value) { value = add(value, 1); }
    void finish() { if (active) { value = add(value, 0xffffffffu); active = false; } }
    ~InitializationDepth() { finish(); }
};
struct FileLease {
    const game::ScenarioStartServices& service;
    FileClass* file;
    ~FileLease() { service.release_file(service.context, file); }
};
template<size_t N> void copy_text(wchar_t (&out)[N], const wchar_t* source) {
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
#define LOCATION(field) if (ini.Exists(s.FileName, #field)) ini.GetInteger(s.FileName, #field, s.field)
    LOCATION(LS640BriefLocX); LOCATION(LS640BriefLocY); LOCATION(LS800BriefLocX); LOCATION(LS800BriefLocY);
#undef LOCATION
    string("LS640BkgdName", s.LS640BkgdName, sizeof(s.LS640BkgdName));
    string("LS800BkgdName", s.LS800BkgdName, sizeof(s.LS800BkgdName));
    string("LS800BkgdPal", s.LS800BkgdPal, sizeof(s.LS800BkgdPal));
}
void scenario_overlay_name(const char* filename, char* output) {
    const char* begin = filename;
    for (const char* p = filename; *p; ++p) if (*p == '/' || *p == '\\' || *p == ':') begin = p + 1;
    const char* end = std::strrchr(begin, '.');
    if (!end) end = begin + std::strlen(begin);
    const size_t count = static_cast<size_t>(end - begin);
    std::memcpy(output, begin, count); std::memcpy(output + count, ".INI", 5);
}
RectangleStruct rectangle(CCINIClass& ini, const char* key, const RectangleStruct& fallback) {
    int values[4], defaults[4]{fallback.X, fallback.Y, fallback.Width, fallback.Height};
    ini.Read4Integers(values, "Map", key, defaults);
    return {values[0], values[1], values[2], values[3]};
}
bool complete(const game::ScenarioRuntimeServices& r) {
    const auto* s = r.initialize;
    return ScenarioClass::Instance && s && s->campaign_difficulty && s->special_flags && s->scenario_crc &&
        s->building_read_flag && s->custom_ai && s->rules_ini && *s->rules_ini && s->ai_ini && s->ui_ini && s->tactical &&
        s->clear_world && s->rules && s->read_country && s->set_map_size && s->set_local_size &&
        s->prepare_mode && s->start_mode && s->start_special_mode && s->mode_ready && s->draw_load_screen &&
        s->destroy_tactical && s->create_tactical && s->tactical_rect && s->theater && s->side && s->find_house &&
        s->read_objects && s->step && s->refresh_cell && s->map_total_value &&
        r.ini && r.ini->progress && r.ini->pump_events && r.houses && r.houses->session && r.houses->countries &&
        r.houses->players && r.houses->houses && r.houses->rules && *r.houses->rules && r.render && r.render->map &&
        r.render->redraw_sidebar && r.world && r.world->initialization_depth && r.world->message_rect &&
        r.world->legacy_starting_units && r.world->screen && r.world->screen->get_manager &&
        r.start && r.start->acquire_file && r.start->release_file;
}
}

bool YRPP_FASTCALL ScenarioClass::InitializeWorldINI(CCINIClass* ini, bool skip_units) {
    const auto& r = game::scenario_runtime();
    if (!ini || !complete(r) || !std::memchr(Instance->FileName, 0, sizeof(Instance->FileName))) return false;
    const auto& s = *r.initialize;
    auto& map = *r.render->map;
    const auto mode = [&] { return r.session_mode(r.context); };
    const auto pump = [&] { r.ini->pump_events(r.ini->context); };
    const auto progress = [&](int value) { r.ini->progress(r.ini->context, value); };
    const auto rules = [&](game::ScenarioRulesPass pass, CCINIClass* source, bool multiplayer = false) {
        s.rules(s.context, pass, *r.houses->rules, source, multiplayer);
    };
    const auto read = [&](game::ScenarioObjectReader reader, CCINIClass* source, bool global = false) {
        s.read_objects(s.context, reader, source, global);
    };
    const auto step = [&](game::ScenarioInitializationStep value) { s.step(s.context, value); };
    const auto player_side = [&](int& side) {
        char player[32]; ini->ReadString("Basic", "Player", "Americans", player);
        HouseClass* house = nullptr;
        if (!s.find_house(s.context, player, house)) return false;
        side = house && house->Type ? house->Type->SideIndex : 0;
        return true;
    };
    InitializationDepth depth(*r.world->initialization_depth);
    const auto fail = [&] { depth.finish(); return false; };
    bool first_campaign_pass = mode() == 0;
    for (;;) {
        if (!first_campaign_pass) s.clear_world(s.context);
        const bool campaign = mode() == 0;
        Instance->Difficulty1 = campaign ? *s.campaign_difficulty : r.houses->session->Config.AIDifficulty;
        Instance->Difficulty2 = 2u - Instance->Difficulty1;
        Instance->SpecialFlags.FogOfWar = !campaign && r.houses->session->Config.FogOfWar;
        s.special_flags->FogOfWar = !campaign && r.houses->session->Config.FogOfWar;
        Instance->InitTime = ini->ReadInteger("Basic", "InitTime", 10000);
        const bool official = ini->ReadBool("Basic", "Official", false);
        CCINIClass auxiliary;
        FileClass* file = nullptr;
        if (!r.start->acquire_file(r.start->context, nullptr, file) || !file) return fail();
        FileLease lease{*r.start, file};
        if (mode() == 0) {
            if (!first_campaign_pass) {
                char filename[sizeof(Instance->FileName) + 5];
                scenario_overlay_name(Instance->FileName, filename);
                file->SetFileName(filename);
                if (file->Exists(false)) {
                    auxiliary.ReadCCFile(file, false, false);
                    rules(game::ScenarioRulesPass::overlay, &auxiliary);
                }
                file->Close();
            }
            file->SetFileName("MISSIONMD.INI");
            if (file->Exists(false)) {
                auxiliary.ReadCCFile(file, false, false);
                mission_metadata(auxiliary);
            }
        }
        std::fill_n(Instance->HouseIndices, 16, -1);
        if (mode() != 0) {
            if (!Instance->ReadWaypoints(*ini)) return fail();
            if (Instance->IsRandom) std::copy_n(Instance->HouseHomeCells, 8, Instance->Waypoints);
            rules(game::ScenarioRulesPass::countries, *s.rules_ini);
            rules(game::ScenarioRulesPass::general, *s.rules_ini);
            for (int i = 0; i < r.houses->countries->Count; ++i) {
                HouseTypeClass* country = nullptr;
                if (!item(r.houses->countries, i, country)) return fail();
                s.read_country(s.context, country, *s.rules_ini);
            }
            AssignHouses();
            s.set_map_size(s.context, rectangle(*ini, "Size", {1, 1, 50, 50}), true, false, true);
            if (!Instance->IsRandom) Instance->ReadStartPoints(*ini);
            s.set_local_size(s.context, rectangle(*ini, "LocalSize", map.VisibleRect));
            auto* game_mode = r.houses->session->MPGameMode;
            if (!game_mode) return fail();
            s.prepare_mode(s.context, game_mode, official);
            if (r.houses->session->unknown_0C == 2) s.start_special_mode(s.context, official);
            else s.start_mode(s.context, r.houses->session->MPGameMode, official);
        }
        LoadProgressManager* manager = nullptr;
        if (!r.world->screen->get_manager(r.world->screen->context, manager) || !manager) return fail();
        s.draw_load_screen(s.context, manager);
        progress(3);
        if (first_campaign_pass) { first_campaign_pass = false; continue; }
        step(game::ScenarioInitializationStep::clear_swizzler);
        if (*s.tactical) s.destroy_tactical(s.context, *s.tactical);
        *s.tactical = nullptr;
        TacticalClass* tactical = nullptr;
        if (!s.create_tactical(s.context, tactical) || !tactical) return fail();
        *s.tactical = tactical;
        s.tactical_rect(s.context, tactical, *r.world->message_rect);
        Instance->Theater = static_cast<TheaterType>(ini->ReadTheater("Map", "Theater", 0));
        s.theater(s.context, static_cast<int>(Instance->Theater));
        progress(30);
        rules(game::ScenarioRulesPass::command_bar, s.ui_ini, mode() != 0 && mode() != 5);
        progress(31);
        rules(game::ScenarioRulesPass::initialize, *s.rules_ini);
        progress(35); pump();
        if (!Instance->ReadGlobalVariables(**s.rules_ini)) return fail();
        pump();
        rules(game::ScenarioRulesPass::overlay, ini);
        progress(45);
        *s.scenario_crc = 0;
        int side = 0;
        if (mode() == 0) {
            // Keep the campaign lookup even though House selection below
            // replaces the result, as the original registry access requires it.
            if (Instance->CampaignIndex != -1) {
                CampaignClass* campaign = nullptr;
                if (!item(r.start->campaigns, Instance->CampaignIndex, campaign)) return fail();
            }
            read(game::ScenarioObjectReader::houses, ini);
            if (!player_side(side)) return fail();
        } else {
            NodeNameType* player = nullptr; HouseTypeClass* country = nullptr;
            if (!item(r.houses->players, 0, player) ||
                !item(r.houses->countries, player->Country == -3 ? 0 : player->Country, country)) return fail();
            side = country->SideIndex;
        }
        Instance->PlayerSideIndex = side;
        if (!s.side(s.context, side)) return fail();
        progress(50);
        if (!Instance->ReadINI(*ini)) return fail();
        progress(58);
        if (mode() == 0) {
            if (!player_side(side) || !s.side(s.context, side)) return fail();
            Instance->PlayerSideIndex = side;
        } else {
            Instance->PlayerSideIndex = side;
            if (r.houses->session->MPGameMode) s.mode_ready(s.context, r.houses->session->MPGameMode);
        }
        if (mode() != 0 && !r.houses->session->Config.BridgeDestruction) s.special_flags->DestroyableBridges = false;
        pump();
        game::read_scenario_type_definitions(s, *ini); progress(60);
        read(game::ScenarioObjectReader::map, ini); pump();
        read(game::ScenarioObjectReader::triggers, ini);
        step(game::ScenarioInitializationStep::building_tiles);
        r.render->redraw_sidebar(r.render->context, 2); progress(70); pump();
        read(game::ScenarioObjectReader::overlays, ini); pump();
        map.CellIteratorReset();
        while (auto* cell = map.CellIteratorNext()) s.refresh_cell(s.context, cell, -1);
        step(game::ScenarioInitializationStep::overlay_bridges);
        read(game::ScenarioObjectReader::terrain, ini); pump();
        step(game::ScenarioInitializationStep::voxel_cache);
        step(game::ScenarioInitializationStep::tile_cache_back); step(game::ScenarioInitializationStep::tile_cache_front);
        progress(72); step(game::ScenarioInitializationStep::radar_surface);
        read(game::ScenarioObjectReader::units, ini); pump(); progress(74);
        read(game::ScenarioObjectReader::aircraft, ini); pump();
        read(game::ScenarioObjectReader::infantry, ini); pump(); progress(76);
        *s.building_read_flag = false;
        read(game::ScenarioObjectReader::buildings, ini); pump(); progress(78);
        *s.building_read_flag = true; pump();
        read(game::ScenarioObjectReader::smudges, ini); pump();
        if (mode() == 5 && *s.custom_ai) {
            file->Close(); file->SetFileName("TMCJ4F.INI");
            if (file->Exists(false)) {
                auxiliary.ReadCCFile(file, false, false);
                rules(game::ScenarioRulesPass::overlay, &auxiliary);
            }
        }
        progress(82); pump();
        int total = 0;
        if (!s.map_total_value(s.context, false, total)) return fail();
        map.TotalValue = total;
        progress(86); pump(); step(game::ScenarioInitializationStep::beacon_art);
        progress(90); pump();
        if (mode() != 0 && !skip_units)
            r.world->legacy_starting_units(r.world->context, ini->ReadBool("Basic", "Official", false));
        pump(); step(game::ScenarioInitializationStep::clear_swizzler); progress(96); pump();
        step(game::ScenarioInitializationStep::expired_objects);
        if (mode() != 0) Instance->SpecialFlags = *s.special_flags;
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
        progress(98); pump(); step(game::ScenarioInitializationStep::clear_radar_cells);
        if (Instance->SpecialFlags.FogOfWar) step(game::ScenarioInitializationStep::fog);
        step(game::ScenarioInitializationStep::release_map_helpers);
        step(game::ScenarioInitializationStep::redraw_map);
        return true;
    }
}
