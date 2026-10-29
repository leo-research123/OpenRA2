/*
 * Read_Scenario / Read_Scenario_INI adapted from EA REDALERT/SCENARIO.CPP,
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae (GPL-3.0-or-later;
 * Copyright 2020 Electronic Arts Inc., third_party/ea/LICENSE.TXT).
 * YR loading and synchronization calibrated to 684370, 684620, 686730.
 * 686B20 and 684C30 share original-class implementations. Starting-unit
 * generation (686890) remains an explicit dependency.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/EvadeClass.h"
#include "yrpp/CampaignClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/WinModemClass.h"
#include "yrpp/WaveClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/FPSCounter.h"
#include "scenario_runtime.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {
int add(int value, unsigned amount) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) + amount); }
void loading(bool value) {
    // 3598 is only written as a byte by these entries; preserve the other
    // three bytes of the historical, unresolved DWORD declaration.
    reinterpret_cast<unsigned char*>(&ScenarioClass::Instance->unknown_3598)[0] = value;
}
bool filename_size(const char* name, size_t& count) {
    if (!name || !*name) return false;
    size_t n = 0;
    while (n < sizeof(ScenarioClass::FileName) && name[n])
        ++n;
    if (n == sizeof(ScenarioClass::FileName)) return false;
    count = n;
    return true;
}
template <class T>
bool item(DynamicVectorClass<T*>* array, int index, T*& result) {
    if (!array || index < 0 || index >= array->Count || !array->Items || !array->Items[index]) return false;
    result = array->Items[index];
    return true;
}
bool random_filename(const char* name, size_t count) {
    if (count < 4) return false;
    const char* extension = name + count - 4;
    return extension[0] == '.' && (extension[1] == 's' || extension[1] == 'S') &&
           (extension[2] == 'e' || extension[2] == 'E') && (extension[3] == 'd' || extension[3] == 'D');
}
int completion_percent() { return ScenarioClass::Instance->IsRandom ? 200 : 100; }
bool sync_services(const game::ScenarioRuntimeServices& r) {
    const auto* w = r.world;
    const auto* n = w ? w->network : nullptr;
    return ScenarioClass::Instance && r.ini && r.ini->pump_events && r.ini->progress && w && w->screen &&
           w->screen->fraction && n && n->poll_input && n->modem_status && n->player_fraction && n->drop_connection &&
           n->is_game_host && n->reset_game_host;
}
bool world_services(const game::ScenarioRuntimeServices& r) {
    if (!ScenarioClass::Instance || !RulesClass::Instance || !r.world || !r.start || !r.ini || !r.ini->pump_events ||
        !r.ini->progress)
        return false;
    const auto& w = *r.world;
    const int mode = static_cast<int>(SessionClass::Instance.GameMode);
    if (mode != 0 && mode != 5 && !sync_services(r)) return false;
    if (mode != 0 && w.screen && (NodeNameType::Array.Count < 1 || NodeNameType::Array.Count > 8)) return false;
    if (w.screen) {
        const auto& s = *w.screen;
        if (!(s.begin && s.get_manager && s.prepare_manager && s.attach_manager && s.set_side && s.prepare_surface &&
              s.campaign_palette && s.load_bar && s.position && s.configure_text && s.extent && s.set_extent &&
              s.fraction && s.set_player_progress && s.finish && s.release_manager))
            return false;
    }
    if (w.network) {
        const auto& n = *w.network;
        const bool no_datagram =
            !n.peer_ports && !n.has_datagram_transport && !n.clear_broadcast_addresses && !n.add_broadcast_address;
        const bool complete_datagram =
            n.peer_ports && n.has_datagram_transport && n.clear_broadcast_addresses && n.add_broadcast_address;
        if (!(no_datagram || complete_datagram) || !n.finish_online_load) return false;
    }
    return w.message_rect && w.load_seed && w.generate_map && w.legacy_starting_units && w.initialize_world_ini &&
           w.legacy_finish_world && w.set_shroud && w.fade_loading_palette && w.show_load_error &&
           w.configure_messages && w.finish_house && r.start->force_disc && r.start->acquire_file &&
           r.start->release_file;
}
}

void ScenarioClass::InitializeDefaultParticleSystem() noexcept {
    if (ParticleSystemClass::DefaultSystem) return;
    if (auto* type = ParticleSystemTypeClass::Find("GasCloudSys"))
        if (auto* memory = YRMemory::Allocate(sizeof(ParticleSystemClass)))
            ParticleSystemClass::DefaultSystem = ::new (memory)
                ParticleSystemClass(type, {2688, 2688, 0}, nullptr, nullptr, CoordStruct::Empty, nullptr);
}

// YR 0x684C30: prepare construction options, publish tags, then rebuild the map.
void YRPP_FASTCALL ScenarioClass::FinishWorld() {
    const auto* host = game::scenario_runtime().world;
    auto& map = MapClass::Instance;
    struct Depth {
        bool active = true;
        Depth() { Unsorted::ScenarioInit = add(Unsorted::ScenarioInit, 1u); }
        void finish() {
            if (active) {
                Unsorted::ScenarioInit = add(Unsorted::ScenarioInit, 0xffffffffu);
                active = false;
            }
        }
        ~Depth() { finish(); }
    } depth;
    for (int i = 0; i < BuildingClass::Array.Count; ++i)
        BuildingClass::Array[i]->UpdateConstructionOptions();
    map.MarkNeedsRedraw(2);
    if (!Unsorted::ArmageddonMode) {
        if (host && host->activate_sidebar) host->activate_sidebar(host->context, 1);
        if (SessionClass::Instance.GameMode == GameMode::Campaign) {
            auto& scenario = *Instance;
            const int index = EvadeClass::Instance.CarryOverGlobals[0] != 0 ? scenario.AltHomeCell : scenario.HomeCell;
            const auto cell = scenario.GetWaypointCoords(index);
            scenario.View4 = scenario.View3 = scenario.View2 = scenario.View1 = cell;
            CoordStruct center{cell.X * 256 + 128, cell.Y * 256 + 128, 0};
            center.Z = map.GetCellFloorHeight(center);
            RadarClass::Instance.vt_entry_D0(&center);
        }
    }
    for (int i = 0; i < TagTypeClass::Array.Count; ++i) {
        auto* type = TagTypeClass::Array[i];
        if (type->GetFlags() & 4) MapClass::PendingTags.AddItem(TagClass::GetInstance(type));
        if (type->GetFlags() & 0x10) LogicClass::PendingTags.AddItem(TagClass::GetInstance(type));
        if (type->GetFlags() & 8) {
            auto* tag = TagClass::GetInstance(type);
            auto* country = tag->Type->FirstTrigger->House;
            if (auto* house = HouseClass::FindByCountryIndex(country->ArrayIndex2)) house->RelatedTags.AddItem(tag);
        }
    }
    depth.finish();
    for (int i = 0; i < TagTypeClass::Array.Count; ++i) {
        auto* type = TagTypeClass::Array[i];
        if (type->HasAllowWinAction()) {
            auto* house = HouseClass::FindByCountryIndex(type->FirstTrigger->House->ArrayIndex);
            house->AllowWinBlocks = add(house->AllowWinBlocks, 1u);
        }
    }
    if (Instance->FillSilos) {
        for (int i = 0; i < HouseClass::Array.Count; ++i)
            if (auto* house = HouseClass::Array[i]) {
                int money = house->Available_Money();
                auto* resource = TiberiumClass::Array[0];
                while (money > resource->Value) {
                    if (house->Available_Storage() <= 0) break;
                    house->GiveTiberium(1.0f, 0);
                    money = add(money, 0u - static_cast<unsigned>(resource->Value));
                }
            }
    }
    DisplayClass::Instance.RestoreSharedSight(true, 0);
    WaveClass::InitializeTables();
    map.CellIteratorReset();
    while (auto* cell = map.CellIteratorNext())
        cell->RecalcAttributes(-1);
    map.ComputeZoneConnections();
    map.ResetAllZones();
    map.ResetAllSubzones();
    map.MarkZoneConnectionCells();
    InitializeDefaultParticleSystem();
    if (RulesClass::Instance->OreTwinkle) {
        map.CellIteratorReset();
        while (auto* cell = map.CellIteratorNext())
            if (cell->GetContainedTiberiumValue() &&
                !Instance->Random.RandomRanged(0, RulesClass::Instance->OreTwinkleChance - 1)) {
                if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
                    ::new (memory)
                        AnimClass(RulesClass::Instance->OreTwinkle, cell->GetCoords(), 0, 1, 0x600, 0, false);
            }
    }
    Detail::SetMinFrameRate(RulesClass::Instance->DetailMinFrameRateNormal);
    Detail::BufferZoneWidth = static_cast<unsigned>(RulesClass::Instance->DetailBufferZoneWidth);
}

bool YRPP_FASTCALL ScenarioClass::ReadScenarioFile(const char* filename) {
    const auto& r = game::scenario_runtime();
    const auto* s = r.start;
    size_t length = 0;
    if (!Instance || !filename_size(filename, length) || !s || !s->force_disc || !s->acquire_file || !s->release_file ||
        !r.world || !r.world->initialize_world_ini)
        return false;
    if (!s->force_disc(s->context)) return false;
    Unsorted::CurrentFrame = 0;
    CCINIClass ini;
    FileClass* file = nullptr;
    if (!s->acquire_file(s->context, filename, file) || !file) return false;
    struct Release {
        const game::ScenarioStartServices& services;
        FileClass* file;
        ~Release() { services.release_file(services.context, file); }
    } lease{*s, file};
    if (!ini.ReadCCFile(file, true, false)) return false;
    std::memmove(Instance->FileName, filename, length + 1);
    return r.world->initialize_world_ini(r.world->context, ini, false);
}

bool YRPP_FASTCALL ScenarioClass::WaitForPlayers() {
    const auto& r = game::scenario_runtime();
    const int mode = static_cast<int>(SessionClass::Instance.GameMode);
    if (mode == 0 || mode == 5) return true;
    if (!sync_services(r)) return false;
    const auto& screen = *r.world->screen;
    const auto& network = *r.world->network;
    const auto& ini = *r.ini;
    (void)SystemTimer::GetTime();  // Original discarded timer initialization.
    SysTimerClass resend(300), timeout(3600);
    double previous = 0;
    if (!screen.fraction(screen.context, previous)) return false;
    for (;;) {
        network.poll_input(network.context);
        ini.pump_events(ini.context);
        int status = 0;
        if (static_cast<int>(SessionClass::Instance.GameMode) == 1 && network.modem_status(network.context, status) &&
            (status & WinModemClass::CarrierDetect) == 0)
            return false;
        if (timeout.Expired()) break;
        double progress = 0;
        if (!screen.fraction(screen.context, progress)) return false;
        if (progress >= 0.9995) {
            ini.progress(ini.context, completion_percent());
            return true;
        }
        // x87 C3 treats unordered as equal here: NaN does not reset timeout.
        if (!std::isnan(progress) && !std::isnan(previous) && progress != previous) {
            timeout.Start(7200);
            previous = progress;
        }
        if (resend.Expired()) {
            resend.Start(60);
            ini.progress(ini.context, completion_percent());
        }
    }
    for (int index = 1, slot = 1; index < NodeNameType::Array.Count; ++slot) {
        double progress = 0;
        if (!network.player_fraction(network.context, slot, progress)) return false;
        if (!(progress >= 0.999)) {  // Unordered is also dropped by x87 C0.
            NodeNameType* player = nullptr;
            if (!item(&NodeNameType::Array, index, player)) return false;
            const int house_index = player->HouseIndex;
            // The original's two name conversions only feed its disabled log.
            network.drop_connection(network.context, house_index, 1);
            HouseClass* house = nullptr;
            bool host = false;
            if (!item(&HouseClass::Array, house_index, house) || !network.is_game_host(network.context, house, host))
                return false;
            if (host) network.reset_game_host(network.context);
        } else
            ++index;
    }
    return false;
}

bool YRPP_FASTCALL ScenarioClass::ReadScenario(const char* filename) {
    const auto& r = game::scenario_runtime();
    size_t length = 0;
    if (!filename_size(filename, length) || !world_services(r)) return false;
    const auto& w = *r.world;
    const game::ScenarioLoadScreenServices no_screen{};
    const game::ScenarioNetworkLoadServices no_network{};
    const auto& screen = w.screen ? *w.screen : no_screen;
    const auto& network = w.network ? *w.network : no_network;
    const auto& ini = *r.ini;
    char source[sizeof(Instance->FileName)];
    std::memcpy(source, filename, length + 1);
    Unsorted::CurrentFrame = 0;
    loading(true);
    Unsorted::ScenarioInit = add(Unsorted::ScenarioInit, 1u);
    Instance->IsRandom = random_filename(source, length);
    LoadProgressManager* manager = nullptr;
    struct LoadScope {
        const game::ScenarioLoadScreenServices& screen;
        LoadProgressManager*& manager;
        bool depth_active = true;
        bool loading_active = true;
        void finish_depth() {
            if (depth_active) {
                Unsorted::ScenarioInit = add(Unsorted::ScenarioInit, 0xffffffffu);
                depth_active = false;
            }
        }
        void finish_loading() {
            if (loading_active) {
                loading(false);
                loading_active = false;
            }
        }
        ~LoadScope() {
            finish_depth();
            finish_loading();
            if (manager) {
                screen.attach_manager(screen.context, nullptr);
                screen.release_manager(screen.context, manager, false);
            }
        }
    } scope{screen, manager};
    const auto fail = [&](bool show_error) {
        if (show_error) {
            w.fade_loading_palette(w.context);
            const wchar_t* ok = game::scenario_text("TXT_OK", 1293);
            const wchar_t* message = game::scenario_text("TXT_UNABLE_READ_SCENARIO", 1293);
            w.show_load_error(w.context, message, ok);
        }
        scope.finish_depth();
        scope.finish_loading();
        if (manager) {
            screen.attach_manager(screen.context, nullptr);
            screen.release_manager(screen.context, manager, false);
            manager = nullptr;
        }
        return false;
    };
    if (w.screen && !Unsorted::ArmageddonMode) {
        const int mode = static_cast<int>(SessionClass::Instance.GameMode);
        screen.begin(screen.context, 100.0,
                     static_cast<unsigned char>(mode == 0 || mode == 5 ? 1 : NodeNameType::Array.Count));
        wchar_t game_id[130];
        const wchar_t* text = nullptr;
        if (static_cast<int>(SessionClass::Instance.GameMode) == 4 && Game::Network::Tournament) {
            // 0x684736: the WOL game id is formatted into the progress text; the result is not checked.
            const wchar_t* format = game::scenario_text("TXT_GAME_ID", 1199);
            std::swprintf(game_id, 130, format, Game::Network::WOLGameID);
            text = game_id;
        }
        if (!screen.get_manager(screen.context, manager) || !manager) return fail(false);
        screen.prepare_manager(screen.context, manager);
        screen.attach_manager(screen.context, manager);
        int side = 0;
        if (static_cast<int>(SessionClass::Instance.GameMode) == 0) {
            if (Instance->CampaignIndex != -1) {
                CampaignClass* campaign = nullptr;
                if (!item(&CampaignClass::Array, Instance->CampaignIndex, campaign)) return fail(false);
                side = campaign->idxCD;
            }
        } else {
            NodeNameType* player = nullptr;
            HouseTypeClass* country = nullptr;
            if (!item(&NodeNameType::Array, 0, player) ||
                !item(&HouseTypeClass::Array, player->Country == -3 ? 0 : player->Country, country))
                return fail(false);
            side = country->SideIndex;
        }
        Instance->PlayerSideIndex = side;
        screen.set_side(screen.context, side);
        LoadProgressManager* live_manager = nullptr;
        if (!screen.get_manager(screen.context, live_manager) || !live_manager) return fail(false);
        screen.prepare_surface(screen.context, live_manager);
        ConvertClass* palette = nullptr;
        const bool campaign = static_cast<int>(SessionClass::Instance.GameMode) == 0;
        if (campaign && !screen.campaign_palette(screen.context, palette)) return fail(false);
        screen.load_bar(screen.context, campaign ? "SPLDBR.SHP" : "PROGBARM.SHP", palette);
        const bool multiplayer = static_cast<int>(SessionClass::Instance.GameMode) != 0;
        Point2D position;
        if (!screen.position(screen.context, manager, position)) return fail(false);
        screen.configure_text(screen.context, position, text, multiplayer, multiplayer);
        int extent = 0;
        if (!screen.extent(screen.context, manager, extent)) return fail(false);
        screen.set_extent(screen.context, extent);
        if (network.has_datagram_transport && network.has_datagram_transport(network.context) &&
            static_cast<int>(SessionClass::Instance.GameMode) == 4 && NodeNameType::Array.Count > 1) {
            network.clear_broadcast_addresses(network.context);
            for (int i = 1; i < NodeNameType::Array.Count; ++i) {
                NodeNameType* player = nullptr;
                if (i >= 8 || !item(&NodeNameType::Array, i, player)) return fail(false);
                unsigned ip = 0;
                static_assert(sizeof(player->Address.sin_addr) == sizeof(ip));
                std::memcpy(&ip, &player->Address.sin_addr, sizeof(ip));
                char address[16];
                std::snprintf(address, sizeof(address), "%u.%u.%u.%u", ip & 255, (ip >> 8) & 255, (ip >> 16) & 255,
                              ip >> 24);
                network.add_broadcast_address(network.context, address, network.peer_ports[i]);
            }
        }
    }
    bool loaded;
    if (Instance->IsRandom) {
        loaded = w.load_seed(w.context, source);
        if (loaded) {
            w.generate_map(w.context);
            w.legacy_starting_units(w.context, true);
        }
        std::memcpy(Instance->FileName, source, length + 1);
    } else
        loaded = ReadScenarioFile(source);
    if (!RulesClass::Instance->Shroud) w.set_shroud(w.context, false);
    if (!loaded) return fail(true);
    w.configure_messages(w.context, add(w.message_rect->X, 3u), w.message_rect->Y,
                         add(w.message_rect->Width, 0xfffffffau));
    w.legacy_finish_world(w.context);
    int mode = static_cast<int>(SessionClass::Instance.GameMode);
    if (mode == 3 || mode == 4) {
        ini.progress(ini.context, 99);
        // 0x684B0A waits up to 15000 ms for the WOL external process (0x664610) when 0xA8DBA0 is set.
        // Native hosts do not run that process, so the wait is not supported and is skipped.
    }
    ini.progress(ini.context, completion_percent());
    SessionClass::Instance.SawCompletion = false;
    SessionClass::Instance.OutOfSync = false;
    const bool synchronized = WaitForPlayers();
    if (static_cast<int>(SessionClass::Instance.GameMode) == 4) network.finish_online_load(network.context);
    scope.finish_depth();
    if (w.screen && !Unsorted::ArmageddonMode && synchronized) {
        ini.pump_events(ini.context);
        for (;;) {
            double progress = 0;
            if (!screen.fraction(screen.context, progress)) {
                scope.finish_loading();
                screen.finish(screen.context);
                if (manager) {
                    screen.attach_manager(screen.context, nullptr);
                    screen.release_manager(screen.context, manager, false);
                    manager = nullptr;
                }
                return false;
            }
            if (progress >= 1.0) break;
            for (int i = 0; i < NodeNameType::Array.Count; ++i)
                screen.set_player_progress(screen.context, i, 100.0,
                                           std::bit_cast<double>(UINT64_C(0xffffffffffffffff)));
        }
    }
    scope.finish_loading();
    if (w.screen) screen.finish(screen.context);
    if (manager) {
        screen.attach_manager(screen.context, nullptr);
        screen.release_manager(screen.context, manager, true);
        manager = nullptr;
    }
    for (int i = HouseClass::Array.Count - 1; i >= 0; --i) {
        HouseClass* house = nullptr;
        if (!item(&HouseClass::Array, i, house)) return false;
        w.finish_house(w.context, house);
    }
    // As in 684620, a multiplayer synchronization failure is not propagated.
    return true;
}
