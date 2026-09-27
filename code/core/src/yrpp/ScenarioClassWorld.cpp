/*
 * Read_Scenario / Read_Scenario_INI adapted from EA REDALERT/SCENARIO.CPP,
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae (GPL-3.0-or-later;
 * Copyright 2020 Electronic Arts Inc., third_party/ea/LICENSE.TXT).
 * YR loading and synchronization calibrated to 684370, 684620, 686730.
 * 686B20 binds the shared initializer; 686890 and 684C30 remain explicit,
 * unported Scenario dependencies.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/CampaignClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/WinModemClass.h"
#include "scenario_runtime.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
int add(int value, unsigned amount) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) + amount);
}
void loading(bool value) {
    // 3598 is only written as a byte by these entries; preserve the other
    // three bytes of the historical, unresolved DWORD declaration.
    reinterpret_cast<unsigned char*>(&ScenarioClass::Instance->unknown_3598)[0] = value;
}
bool filename_size(const char* name, size_t& count) {
    if (!name || !*name) return false;
    size_t n = 0;
    while (n < sizeof(ScenarioClass::FileName) && name[n]) ++n;
    if (n == sizeof(ScenarioClass::FileName)) return false;
    count = n;
    return true;
}
template<class T> bool item(DynamicVectorClass<T*>* array, int index, T*& result) {
    if (!array || index < 0 || index >= array->Count || !array->Items || !array->Items[index]) return false;
    result = array->Items[index]; return true;
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
    return ScenarioClass::Instance && r.ini && r.ini->pump_events && r.ini->progress &&
        r.houses && r.houses->players && r.houses->houses && w && w->screen && w->screen->fraction &&
        n && n->poll_input && n->modem_status && n->player_fraction && n->drop_connection &&
        n->is_game_host && n->reset_game_host;
}
bool world_services(const game::ScenarioRuntimeServices& r) {
    if (!sync_services(r) || !r.start || !r.ini->armageddon_mode || !r.houses->session ||
        !r.houses->countries || !r.houses->rules || !*r.houses->rules) return false;
    // Campaign loading uses CampaignClass for its side and a single progress
    // slot (6846EB/684782); it does not require a multiplayer NodeName list.
    // Other modes use players[0] and the fixed eight-player network buffers.
    if (r.session_mode(r.context) != 0 &&
        (r.houses->players->Count < 1 || r.houses->players->Count > 8)) return false;
    const auto& w = *r.world; const auto& s = *w.screen; const auto& n = *w.network;
    const bool no_datagram = !n.peer_ports && !n.has_datagram_transport &&
        !n.clear_broadcast_addresses && !n.add_broadcast_address;
    const bool complete_datagram = n.peer_ports && n.has_datagram_transport &&
        n.clear_broadcast_addresses && n.add_broadcast_address;
    return w.initialization_depth && w.message_rect && w.load_seed && w.generate_map &&
        w.legacy_starting_units && w.initialize_world_ini && w.legacy_finish_world &&
        w.set_shroud && w.fade_loading_palette && w.show_load_error && w.configure_messages && w.finish_house &&
        s.tournament && s.game_id && s.format_game_id && s.begin && s.get_manager && s.prepare_manager &&
        s.attach_manager && s.set_side && s.prepare_surface && s.campaign_palette && s.load_bar &&
        s.position && s.configure_text && s.extent && s.set_extent && s.set_player_progress && s.finish && s.release_manager &&
        n.delay_before_sync && (no_datagram || complete_datagram) && n.delay && n.finish_online_load &&
        r.start->force_disc && r.start->acquire_file && r.start->release_file;
}
}

bool YRPP_FASTCALL ScenarioClass::ReadScenarioFile(const char* filename) {
    const auto& r = game::scenario_runtime();
    const auto* s = r.start;
    size_t length = 0;
    if (!Instance || !filename_size(filename, length) || !s || !s->force_disc || !s->acquire_file ||
        !s->release_file || !r.world || !r.world->initialize_world_ini) return false;
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
    const int mode = r.session_mode(r.context);
    if (mode == 0 || mode == 5) return true;
    if (!sync_services(r)) return false;
    const auto& screen = *r.world->screen;
    const auto& network = *r.world->network;
    const auto& ini = *r.ini;
    (void)SystemTimer::GetTime(); // Original discarded timer initialization.
    SysTimerClass resend(300), timeout(3600);
    double previous = 0;
    if (!screen.fraction(screen.context, previous)) return false;
    for (;;) {
        network.poll_input(network.context);
        ini.pump_events(ini.context);
        int status = 0;
        if (r.session_mode(r.context) == 1 && network.modem_status(network.context, status) &&
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
    for (int index = 1, slot = 1; index < r.houses->players->Count; ++slot) {
        double progress = 0;
        if (!network.player_fraction(network.context, slot, progress)) return false;
        if (!(progress >= 0.999)) { // Unordered is also dropped by x87 C0.
            NodeNameType* player = nullptr;
            if (!item(r.houses->players, index, player)) return false;
            const int house_index = player->HouseIndex;
            // The original's two name conversions only feed its disabled log.
            network.drop_connection(network.context, house_index, 1);
            HouseClass* house = nullptr;
            bool host = false;
            if (!item(r.houses->houses, house_index, house) ||
                !network.is_game_host(network.context, house, host)) return false;
            if (host) network.reset_game_host(network.context);
        } else ++index;
    }
    return false;
}

bool YRPP_FASTCALL ScenarioClass::ReadScenario(const char* filename) {
    const auto& r = game::scenario_runtime();
    size_t length = 0;
    if (!filename_size(filename, length) || !world_services(r)) return false;
    const auto& w = *r.world;
    const auto& screen = *w.screen;
    const auto& network = *w.network;
    const auto& ini = *r.ini;
    char source[sizeof(Instance->FileName)];
    std::memcpy(source, filename, length + 1);
    Unsorted::CurrentFrame = 0;
    loading(true);
    *w.initialization_depth = add(*w.initialization_depth, 1u);
    Instance->IsRandom = random_filename(source, length);
    LoadProgressManager* manager = nullptr;
    const auto fail = [&](bool show_error) {
        if (show_error) {
            w.fade_loading_palette(w.context);
            const wchar_t* ok = game::scenario_text("TXT_OK", 1293);
            const wchar_t* message = game::scenario_text("TXT_UNABLE_READ_SCENARIO", 1293);
            w.show_load_error(w.context, message, ok);
        }
        *w.initialization_depth = add(*w.initialization_depth, 0xffffffffu);
        loading(false);
        if (manager) {
            screen.attach_manager(screen.context, nullptr);
            screen.release_manager(screen.context, manager, false);
        }
        return false;
    };
    if (!*ini.armageddon_mode) {
        const int mode = r.session_mode(r.context);
        screen.begin(screen.context, 100.0, static_cast<unsigned char>(mode == 0 || mode == 5 ? 1 : r.houses->players->Count));
        wchar_t game_id[130];
        const wchar_t* text = nullptr;
        if (r.session_mode(r.context) == 4 && *screen.tournament) {
            const wchar_t* format = game::scenario_text("TXT_GAME_ID", 1199);
            if (!screen.format_game_id(screen.context, format, *screen.game_id, game_id, 130)) return fail(false);
            text = game_id;
        }
        if (!screen.get_manager(screen.context, manager) || !manager) return fail(false);
        screen.prepare_manager(screen.context, manager);
        screen.attach_manager(screen.context, manager);
        int side = 0;
        if (r.session_mode(r.context) == 0) {
            if (Instance->CampaignIndex != -1) {
                CampaignClass* campaign = nullptr;
                if (!item(r.start->campaigns, Instance->CampaignIndex, campaign)) return fail(false);
                side = campaign->idxCD;
            }
        } else {
            NodeNameType* player = nullptr;
            HouseTypeClass* country = nullptr;
            if (!item(r.houses->players, 0, player) ||
                !item(r.houses->countries, player->Country == -3 ? 0 : player->Country, country)) return fail(false);
            side = country->SideIndex;
        }
        Instance->PlayerSideIndex = side;
        screen.set_side(screen.context, side);
        LoadProgressManager* live_manager = nullptr;
        if (!screen.get_manager(screen.context, live_manager) || !live_manager) return fail(false);
        screen.prepare_surface(screen.context, live_manager);
        ConvertClass* palette = nullptr;
        const bool campaign = r.session_mode(r.context) == 0;
        if (campaign && !screen.campaign_palette(screen.context, palette)) return fail(false);
        screen.load_bar(screen.context, campaign ? "SPLDBR.SHP" : "PROGBARM.SHP", palette);
        const bool multiplayer = r.session_mode(r.context) != 0;
        Point2D position;
        if (!screen.position(screen.context, manager, position)) return fail(false);
        screen.configure_text(screen.context, position, text, multiplayer, multiplayer);
        int extent = 0;
        if (!screen.extent(screen.context, manager, extent)) return fail(false);
        screen.set_extent(screen.context, extent);
        if (network.has_datagram_transport && network.has_datagram_transport(network.context) &&
            r.session_mode(r.context) == 4 && r.houses->players->Count > 1) {
            network.clear_broadcast_addresses(network.context);
            for (int i = 1; i < r.houses->players->Count; ++i) {
                NodeNameType* player = nullptr;
                if (i >= 8 || !item(r.houses->players, i, player)) return fail(false);
                unsigned ip = 0;
                static_assert(sizeof(player->Address.sin_addr) == sizeof(ip));
                std::memcpy(&ip, &player->Address.sin_addr, sizeof(ip));
                char address[16];
                std::snprintf(address, sizeof(address), "%u.%u.%u.%u", ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24);
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
    } else loaded = ReadScenarioFile(source);
    if (!(*r.houses->rules)->Shroud) w.set_shroud(w.context, false);
    if (!loaded) return fail(true);
    w.configure_messages(w.context, add(w.message_rect->X, 3u), w.message_rect->Y, add(w.message_rect->Width, 0xfffffffau));
    w.legacy_finish_world(w.context);
    int mode = r.session_mode(r.context);
    if (mode == 3 || mode == 4) {
        ini.progress(ini.context, 99);
        if (r.session_mode(r.context) == 4 && *network.delay_before_sync)
            network.delay(network.context, 15000, true);
    }
    ini.progress(ini.context, completion_percent());
    r.houses->session->SawCompletion = false;
    r.houses->session->OutOfSync = false;
    const bool synchronized = WaitForPlayers();
    if (r.session_mode(r.context) == 4) network.finish_online_load(network.context);
    *w.initialization_depth = add(*w.initialization_depth, 0xffffffffu);
    if (!*ini.armageddon_mode && synchronized) {
        ini.pump_events(ini.context);
        for (;;) {
            double progress = 0;
            if (!screen.fraction(screen.context, progress)) {
                loading(false);
                screen.finish(screen.context);
                if (manager) {
                    screen.attach_manager(screen.context, nullptr);
                    screen.release_manager(screen.context, manager, false);
                }
                return false;
            }
            if (progress >= 1.0) break;
            for (int i = 0; i < r.houses->players->Count; ++i)
                screen.set_player_progress(screen.context, i, 100.0, std::bit_cast<double>(UINT64_C(0xffffffffffffffff)));
        }
    }
    loading(false);
    screen.finish(screen.context);
    if (manager) {
        screen.attach_manager(screen.context, nullptr);
        screen.release_manager(screen.context, manager, true);
    }
    for (int i = r.houses->houses->Count - 1; i >= 0; --i) {
        HouseClass* house = nullptr;
        if (!item(r.houses->houses, i, house)) return false;
        w.finish_house(w.context, house);
    }
    // As in 684620, a multiplayer synchronization failure is not propagated.
    return true;
}
