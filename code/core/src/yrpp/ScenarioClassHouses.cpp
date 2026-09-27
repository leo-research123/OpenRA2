// Scenario house assignment keeps the YRpp original House/Session model.
// YR 687F10; RA1 SCENARIO.CPP's fixed-house assignment is version-specific,
// so the target's dynamically constructed multiplayer houses are retained.
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ColorScheme.h"
#include "scenario_runtime.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <cwchar>
#include <stdexcept>

namespace {
const game::ScenarioHouseServices& world() {
    const auto* world = game::scenario_runtime().houses;
    if (!ScenarioClass::Instance || !world || !world->session || !world->players || !world->countries ||
        !world->houses || !world->current_player || !world->observer || !world->rules || !*world->rules || !world->tech_level ||
        !world->color_lookup || !world->create_house || !world->update_house_color ||
        !world->update_laser_color || !world->assign_handicap)
        throw std::logic_error("Scenario house assignment requires the existing world and session objects");
    return *world;
}
HouseClass* create_house(const game::ScenarioHouseServices& world, int country) {
    if (!world.countries->ValidIndex(country) || !world.countries->Items[country])
        throw std::runtime_error("Scenario references a missing country");
    HouseClass* house = nullptr;
    if (!world.create_house(world.context, world.countries->Items[country], house) || !house)
        throw std::runtime_error("Scenario could not create its House");
    return house;
}
int country_index(const game::ScenarioHouseServices& world, const char* name) {
    // 5117D0 checks the display name before ID and returns ArrayIndex2.
    for (int i = 0; i < world.countries->Count; ++i) {
        const auto* country = world.countries->Items[i];
        if (!_strcmpi(country->Name, name) || !_strcmpi(country->ID, name)) return country->ArrayIndex2;
    }
    return -1;
}
int color_index(const game::ScenarioHouseServices& world, int slot) {
    if (slot == -2) return world.color_lookup[8];
    return static_cast<unsigned>(slot) < 9 ? world.color_lookup[slot] : slot;
}
void colors(const game::ScenarioHouseServices& world, HouseClass* house, int slot) {
    house->ColorSchemeIndex = color_index(world, slot);
    world.update_house_color(world.context, house);
    world.update_laser_color(world.context, house);
}
template<std::size_t N> void name(char (&out)[N], const char* value) {
    std::strncpy(out, value, N - 1); out[N - 1] = 0;
}
int neutral_color() {
    const auto* render = game::scenario_runtime().render;
    if (!render || !render->color_schemes) throw std::logic_error("Scenario requires the color-scheme registry");
    for (int i = 0; i < render->color_schemes->Count; ++i) {
        const auto* scheme = render->color_schemes->Items[i];
        if (scheme->ShadeCount == 53 && !_strcmpi(scheme->ID, "LightGrey")) return i;
    }
    return -1; // House's palette update supplies the original fallback.
}
}

void YRPP_FASTCALL ScenarioClass::AssignHouses() {
    const auto& data = world();
    const int count = data.players->Count;
    if (static_cast<unsigned>(count) > 8u) throw std::runtime_error("Scenario supports at most eight human player slots");
    for (int i = 0; i < count; ++i) {
        if (!data.players->Items[i]) throw std::runtime_error("Scenario has an empty player slot");
        if (data.players->Items[i]->Country == -3) data.players->Items[i]->SetCountry(0);
    }
    std::array<bool, 8> assigned{};
    *data.observer = nullptr;
    for (int remaining = count; remaining; --remaining) {
        int chosen = -1, color = -1;
        for (int i = 0; i < count; ++i) {
            if (!assigned[i] && (color == -1 || data.players->Items[i]->Color < color)) {
                chosen = i; color = data.players->Items[i]->Color;
            }
        }
        auto* player = data.players->Items[chosen];
        assigned[chosen] = true;
        auto* house = create_house(data, player->Country);
        if (data.session->GameMode == GameMode::Internet) {
            // Original WString conversion (735090) retains the low byte of
            // each UTF-16 unit. A zero low byte ends the resulting C string.
            char narrow[21]{};
            for (int i = 0; i < 20 && player->Name[i]; ++i) narrow[i] = static_cast<char>(player->Name[i]);
            name(house->PlainName, narrow);
        } else name(house->PlainName, "<human player>");
        std::wcsncpy(house->UIName, player->Name, 20); house->UIName[20] = 0;
        house->IsHumanPlayer = true;
        house->InitializeForMultiplayer(player->Color, player->Country, data.session->Config.Money);
        colors(data, house, player->Color);
        house->StartingPoint = player->GetStartPoint();
        house->StartingAllies.data = static_cast<DWORD>(player->Team);
        if (chosen == 0) { *data.current_player = house; house->IsInPlayerControl = true; }
        if (player->SpectatorFlag == 0xffffffffu) *data.observer = house;
        house->TechLevel = *data.tech_level;
        data.assign_handicap(data.context, house, 1);
        player->HouseIndex = house->ArrayIndex;
    }
    int created_ai = 0;
    const int requested_ai = data.session->Config.AIPlayers;
    const auto& slots = data.session->Config.AISlots;
    for (int slot = 0; slot < 8; ++slot) {
        const int country = slots.Countries[slot];
        if (created_ai >= requested_ai || country == -1 || country == -3) continue;
        ++created_ai;
        auto* house = create_house(data, country);
        house->IsHumanPlayer = false;
        house->TechLevel = *data.tech_level;
        house->InitializeForMultiplayer(slots.Colors[slot], country, data.session->Config.Money);
        colors(data, house, slots.Colors[slot]);
        house->StartingPoint = slots.Starts[slot];
        house->StartingAllies.data = static_cast<DWORD>(slots.Allies[slot]);
        if (slots.Allies[slot] != -1) Instance->TeamsPresent = true;
        name(house->PlainName, "Computer");
        const wchar_t* text = game::scenario_text("TXT_COMPUTER", 4032);
        std::wcsncpy(house->UIName, text, 20); house->UIName[20] = 0;
        if (data.session->GameMode != GameMode::Campaign) house->IQLevel2 = (*data.rules)->MaxIQLevels;
        int difficulty = slots.Difficulties[slot];
        if (data.players->Count > 1 && (*data.rules)->CompEasyBonus && difficulty > 0) --difficulty;
        data.assign_handicap(data.context, house, difficulty);
    }
    for (const auto* name : {"Neutral", "Special"}) {
        auto* house = create_house(data, country_index(data, name));
        house->ColorSchemeIndex = neutral_color();
        data.update_house_color(data.context, house);
    }
}
