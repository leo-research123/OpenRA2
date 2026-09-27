/*
 * Start_Scenario adapted from EA REDALERT/SCENARIO.CPP at
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae (GPL-3.0-or-later;
 * Copyright 2020 Electronic Arts Inc., third_party/ea/LICENSE.TXT).
 * YR ordering and branches calibrated to 683AB0..683EAF. The world-loading
 * controller is shared core code; world initialization remains incomplete.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/CampaignClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/Surface.h"
#include "scenario_runtime.hpp"
#include <bit>
#include <cstdint>
#include <cstring>

namespace {
bool complete(const game::ScenarioStartServices* s) {
    return s && s->session && s->media_check_depth && s->current_resolution &&
        s->preferred_resolution && s->hidden_surface && *s->hidden_surface &&
        s->scenario_started && s->game_active && s->request_disc && s->current_disc &&
        s->mission_has_disc && s->mission_first_disc && s->force_disc &&
        s->hide_cursor && s->show_cursor && s->acquire_file && s->release_file &&
        s->play_movie && s->stop_theme && s->find_theme && s->play_theme && s->queue_theme &&
        s->load_world && s->dropship_dialog && s->resize_display &&
        s->release_menu_assets && s->apply_options && s->present_surface &&
        s->disable_ime && s->clear_online_state;
}
template<class T> bool item(DynamicVectorClass<T*>* list, int index, T*& result) {
    if (!list || index < 0 || index >= list->Count || !list->Items || !list->Items[index]) return false;
    result = list->Items[index];
    return true;
}
void add_wrapping(int& value, unsigned increment) {
    value = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) + increment);
}
struct FileLease {
    const game::ScenarioStartServices& services;
    FileClass* file;
    ~FileLease() { services.release_file(services.context, file); }
};
}

bool YRPP_FASTCALL ScenarioClass::StartScenario(const char* filename, bool briefing, int campaign_index) {
    const auto& runtime = game::scenario_runtime();
    const auto* services = runtime.start;
    if (!Instance || !complete(services)) return false;
    const auto& s = *services;
    const auto clear_online = [&] {
        if (runtime.session_mode(runtime.context) == 4) s.clear_online_state(s.context);
    };
    const auto fail_after_hide = [&] {
        s.show_cursor(s.context);
        clear_online();
        return false;
    };

    if ((!filename || !*filename) && campaign_index != -1) {
        if (!s.alternate_campaign) return false;
        if (*s.alternate_campaign == 1) filename = s.alternate_filename;
        else {
            CampaignClass* campaign = nullptr;
            if (!item(s.campaigns, campaign_index, campaign)) return false;
            filename = campaign->Scenario;
        }
    }
    if (!filename || !*filename) return false;
    size_t length = 0;
    while (length < sizeof(Instance->FileName) && filename[length]) ++length;
    if (length == sizeof(Instance->FileName)) return false;
    Instance->CampaignIndex = campaign_index;
    // Keep the caller's spelling for file I/O. Aliasing FileName itself makes
    // subsequent I/O see the uppercase value, as in the original.
    std::memmove(Instance->FileName, filename, length + 1);
    for (size_t i = 0; i < length; ++i) {
        char& c = Instance->FileName[i];
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    s.request_disc(s.context, -1);
    if (runtime.session_mode(runtime.context) == 0) {
        if (Instance->CampaignIndex != -1) {
            CampaignClass* campaign = nullptr;
            if (!item(s.campaigns, Instance->CampaignIndex, campaign)) return false;
            s.request_disc(s.context, campaign->idxCD);
        }
    } else if (s.session->Config.ScenarioIndex != -1) {
        MultiMission* mission = nullptr;
        if (!item(&s.session->MultiMission, s.session->Config.ScenarioIndex, mission)) return false;
        int disc = -1;
        bool available = false;
        if (!s.current_disc(s.context, 60, disc) || !s.mission_has_disc(s.context, mission, disc, available)) return false;
        if (!available) {
            if (!s.mission_first_disc(s.context, mission, disc)) return false;
            s.request_disc(s.context, disc);
        }
    }
    add_wrapping(*s.media_check_depth, 1u);
    if (!s.force_disc(s.context)) {
        // 683BF6 jumps past the decrement on failure. Preserve that state.
        clear_online();
        return false;
    }
    add_wrapping(*s.media_check_depth, 0xffffffffu);
    s.hide_cursor(s.context);
    {
        CCINIClass ini;
        FileClass* file = nullptr;
        if (!s.acquire_file(s.context, filename, file) || !file) return fail_after_hide();
        FileLease lease{s, file}; // File goes away before the INI object.
        // A return value of 2 (missing/mismatched digest) is accepted by 683C6C.
        if (ini.ReadCCFile(file, true, false)) {
            int movie = ini.ReadMovie("Basic", "Intro", -1);
            if (movie == -1) movie = ini.ReadMovie("Basic", "Brief", -1);
            if (movie != -1) {
                s.stop_theme(s.context, false);
                s.play_movie(s.context, movie, -1, true, true, true);
            }
        }
    }
    int loading_theme = -1;
    if (!s.find_theme(s.context, "LOADING", loading_theme)) return fail_after_hide();
    s.play_theme(s.context, loading_theme);
    if (!s.load_world(s.context, filename)) return fail_after_hide();
    // The target's Brief filename formatting has no consumer or state effect.
    if (Instance->StartingDropships > 0) s.dropship_dialog(s.context);
    if (briefing) s.play_movie(s.context, Instance->Action, -1, true, true, true);
    if (s.current_resolution->X != s.preferred_resolution->X || s.current_resolution->Y != s.preferred_resolution->Y)
        s.resize_display(s.context, s.preferred_resolution->X, s.preferred_resolution->Y);
    s.release_menu_assets(s.context);
    s.apply_options(s.context);
    (*s.hidden_surface)->Fill(0);
    s.present_surface(s.context, *s.hidden_surface, true);
    Instance->ElapsedTimer.Resume();
    if (Instance->ThemeIndex == -1) s.stop_theme(s.context, true);
    else s.queue_theme(s.context, Instance->ThemeIndex);
    s.disable_ime(s.context);
    *s.scenario_started = true;
    *s.game_active = true;
    clear_online();
    return true;
}
