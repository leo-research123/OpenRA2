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
void require_world() {
    if (!ScenarioClass::Instance || !RulesClass::Instance)
        throw std::logic_error("Scenario house assignment requires the existing world and session objects");
}
HouseClass* create_house(int country) {
    if (!HouseTypeClass::Array.ValidIndex(country) || !HouseTypeClass::Array.Items[country])
        throw std::runtime_error("Scenario references a missing country");
    // 0x4F54A0 registers the new House in HouseClass::Array.
    return new HouseClass(HouseTypeClass::Array.Items[country]);
}
int country_index(const char* name) {
    // 5117D0 checks the display name before ID and returns ArrayIndex2.
    for (int i = 0; i < HouseTypeClass::Array.Count; ++i) {
        const auto* country = HouseTypeClass::Array.Items[i];
        if (!_strcmpi(country->Name, name) || !_strcmpi(country->ID, name)) return country->ArrayIndex2;
    }
    return -1;
}
int color_index(int slot) {
    if (slot == -2) return ColorScheme::PlayerColorToColorSchemeLUT[8];
    return static_cast<unsigned>(slot) < 9 ? ColorScheme::PlayerColorToColorSchemeLUT[slot] : slot;
}
void colors(HouseClass* house, int slot) {
    house->ColorSchemeIndex = color_index(slot);
    house->UpdateColor();
    house->UpdateLaserColor();
}
template <std::size_t N>
void name(char (&out)[N], const char* value) {
    std::strncpy(out, value, N - 1);
    out[N - 1] = 0;
}
int neutral_color() {
    for (int i = 0; i < ColorScheme::Array.Count; ++i) {
        const auto* scheme = ColorScheme::Array.Items[i];
        if (scheme->ShadeCount == 53 && !_strcmpi(scheme->ID, "LightGrey")) return i;
    }
    return -1;  // House's palette update supplies the original fallback.
}
}

void YRPP_FASTCALL ScenarioClass::AssignHouses() {
    require_world();
    const int count = NodeNameType::Array.Count;
    if (static_cast<unsigned>(count) > 8u)
        throw std::runtime_error("Scenario supports at most eight human player slots");
    for (int i = 0; i < count; ++i) {
        if (!NodeNameType::Array.Items[i]) throw std::runtime_error("Scenario has an empty player slot");
        if (NodeNameType::Array.Items[i]->Country == -3) NodeNameType::Array.Items[i]->SetCountry(0);
    }
    std::array<bool, 8> assigned{};
    HouseClass::Observer = nullptr;
    for (int remaining = count; remaining; --remaining) {
        int chosen = -1, color = -1;
        for (int i = 0; i < count; ++i) {
            if (!assigned[i] && (color == -1 || NodeNameType::Array.Items[i]->Color < color)) {
                chosen = i;
                color = NodeNameType::Array.Items[i]->Color;
            }
        }
        auto* player = NodeNameType::Array.Items[chosen];
        assigned[chosen] = true;
        auto* house = create_house(player->Country);
        if (SessionClass::Instance.GameMode == GameMode::Internet) {
            // Original WString conversion (735090) retains the low byte of
            // each UTF-16 unit. A zero low byte ends the resulting C string.
            char narrow[21]{};
            for (int i = 0; i < 20 && player->Name[i]; ++i)
                narrow[i] = static_cast<char>(player->Name[i]);
            name(house->PlainName, narrow);
        } else
            name(house->PlainName, "<human player>");
        std::wcsncpy(house->UIName, player->Name, 20);
        house->UIName[20] = 0;
        house->IsHumanPlayer = true;
        house->InitializeForMultiplayer(player->Color, player->Country, SessionClass::Instance.Config.Money);
        colors(house, player->Color);
        house->StartingPoint = player->GetStartPoint();
        house->StartingAllies.data = static_cast<DWORD>(player->Team);
        if (chosen == 0) {
            HouseClass::CurrentPlayer = house;
            house->IsInPlayerControl = true;
        }
        if (player->SpectatorFlag == 0xffffffffu) HouseClass::Observer = house;
        house->TechLevel = Game::TechLevel;
        house->AssignHandicap(1);
        player->HouseIndex = house->ArrayIndex;
    }
    int created_ai = 0;
    const int requested_ai = SessionClass::Instance.Config.AIPlayers;
    const auto& slots = SessionClass::Instance.Config.AISlots;
    for (int slot = 0; slot < MaxPlayerSlots; ++slot) {
        const int country = slots.Countries[slot];
        if (created_ai >= requested_ai || country == -1 || country == -3) continue;
        ++created_ai;
        auto* house = create_house(country);
        house->IsHumanPlayer = false;
        house->TechLevel = Game::TechLevel;
        house->InitializeForMultiplayer(slots.Colors[slot], country, SessionClass::Instance.Config.Money);
        colors(house, slots.Colors[slot]);
        house->StartingPoint = slots.Starts[slot];
        house->StartingAllies.data = static_cast<DWORD>(slots.Allies[slot]);
        if (slots.Allies[slot] != -1) Instance->TeamsPresent = true;
        name(house->PlainName, "Computer");
        const wchar_t* text = game::scenario_text("TXT_COMPUTER", 4032);
        std::wcsncpy(house->UIName, text, 20);
        house->UIName[20] = 0;
        if (SessionClass::Instance.GameMode != GameMode::Campaign) house->IQLevel2 = RulesClass::Instance->MaxIQLevels;
        int difficulty = slots.Difficulties[slot];
        if (NodeNameType::Array.Count > 1 && RulesClass::Instance->CompEasyBonus && difficulty > 0) --difficulty;
        house->AssignHandicap(difficulty);
    }
    for (const auto* name : {"Neutral", "Special"}) {
        auto* house = create_house(country_index(name));
        house->ColorSchemeIndex = neutral_color();
        house->UpdateColor();
    }
    // Alliance, cloak and base-spacing masks hold one bit per House and wrap
    // larger indices onto other Houses.
    if (HouseClass::Array.Count > MaxHouseBits) throw std::runtime_error("Scenario has more Houses than house masks hold");
}
