// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 house.cpp Read_INI; YR house-list
// reader 0x005009B0 and per-house reader 0x00500B40.
#include "yrpp/SessionClass.h"
#include "scenario_runtime.hpp"
#include "x87_integer.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
namespace {
void read_house_colors(HouseClass& house) {
    // 0x00500DD9..0x00501086: select the lit scheme and decode the converter's
    // main shade before deriving laser RGB. The original indexes the scheme
    // table without a bound check; an index past it counts as a missing scheme.
    auto* scheme = ColorScheme::Array.GetItemOrDefault(house.ColorSchemeIndex);
    if (!scheme) {
        house.ColorSchemeIndex = 5;
        scheme = ColorScheme::Array.GetItemOrDefault(house.ColorSchemeIndex);
    }
    if (!scheme || !scheme->LightConvert || !scheme->LightConvert->PaletteData)
        throw std::runtime_error("House color scheme is unavailable");
    const auto& convert = *scheme->LightConvert;
    const unsigned pixel = convert.BytesPerPixel == 1
                               ? static_cast<const BYTE*>(convert.PaletteData)[scheme->MainShadeIndex]
                               : static_cast<const WORD*>(convert.PaletteData)[scheme->MainShadeIndex];
    house.Color = Drawing::Int_To_RGB(pixel);
    double red = house.Color.R, green = house.Color.G, blue = house.Color.B;
    const double length = std::sqrt(blue * blue + green * green + red * red);
    if (length == 0.0) {
        red = green = blue = 255.0;
    } else {
        red = std::min(red * 240.0 / length, 255.0);
        green = std::min(green * 240.0 / length, 255.0);
        blue = std::min(blue * 240.0 / length, 255.0);
        if (red < 96.0) red = 0.0;
        if (green < 96.0) green = 0.0;
        if (blue < 96.0) blue = 0.0;
    }
    // 0x0050B920 normalizes a second time after the weak channels are removed.
    const double laserLength = std::sqrt(red * red + blue * blue + green * green);
    if (laserLength == 0.0) {
        red = green = blue = 255.0;
    } else {
        red = std::min(red * 240.0 / laserLength, 255.0);
        green = std::min(green * 240.0 / laserLength, 255.0);
        blue = std::min(blue * 240.0 / laserLength, 255.0);
    }
    // The original converts blue, green, then red and leaves x87 in chop mode.
    const auto laserBlue = static_cast<BYTE>(game::x87_integer(blue));
    const auto laserGreen = static_cast<BYTE>(game::x87_integer(green));
    const auto laserRed = static_cast<BYTE>(game::x87_integer(red));
    house.LaserColor = {laserRed, laserGreen, laserBlue};
}
// The original copies UIName with wcsncpy(UIName, text, 32), padding with
// zeros, or with an unbounded wcscpy of the country name. Its 16-bit units
// past the 21 of UIName land on ColorSchemeIndex, StartingPoint,
// StartingAllies and unknown_16060; units 29..31 reach the WaypointPath vector
// header and are not written here.
void store_ui_name(HouseClass& house, const wchar_t* text, bool padded) {
    std::uint16_t units[32]{};
    int written = 0;
    if (padded) {
        for (int i = 0; i < 32 && text && text[i]; ++i)
            units[i] = static_cast<std::uint16_t>(text[i]);
        written = 32;
    } else {
        while (text && text[written]) {
            units[written] = static_cast<std::uint16_t>(text[written]);
            if (++written == 31) break;
        }
        ++written;
    }
    void* overflow[]{&house.ColorSchemeIndex, &house.StartingPoint, &house.StartingAllies.data,
                     &house.unknown_16060};
    for (int i = 0; i < written && i < 29; ++i) {
        if (i < 21) {
            house.UIName[i] = units[i];
            continue;
        }
        auto* target = overflow[(i - 21) / 2];
        std::uint32_t field;
        std::memcpy(&field, target, sizeof(field));
        field = (i - 21) % 2 ? (field & 0xFFFFu) | (std::uint32_t(units[i]) << 16) : (field & 0xFFFF0000u) | units[i];
        std::memcpy(target, &field, sizeof(field));
    }
}
bool campaign() { return SessionClass::Instance.GameMode == GameMode::Campaign; }
}  // namespace

void HouseClass::LoadFromINI(CCINIClass* ini) {
    char name[21];
    std::memcpy(name, PlainName, sizeof(name));
    name[20] = 0;
    // 0x00526B00: the section lookup is cached by name pointer, and this name
    // sits at the same stack address for every house.
    ini->Reset();
    auto& rules = *RulesClass::Instance;
    TechLevel = ini->ReadInteger(name, "TechLevel", ScenarioClass::Instance->TechLevel);
    StartingCredits = 100 * ini->ReadInteger(name, "Credits", 0);
    IsInPlayerControl = ini->ReadBool(name, "PlayerControl", false);
    if (IsInPlayerControl && campaign()) {
        // 0x00A8EB64 is the campaign difficulty, which 0x00686B78 also stores as
        // Difficulty1; a non-positive sum becomes 0.
        int balance = StartingCredits;
        if (ScenarioClass::Instance->Difficulty1 == 0)
            balance += rules.CampaignMoneyDeltaEasy;
        else if (ScenarioClass::Instance->Difficulty1 == 2)
            balance += rules.CampaignMoneyDeltaHard;
        Balance = balance <= 0 ? 0 : balance;
    } else {
        Balance = StartingCredits;
    }
    char label[32];
    ini->ReadString(name, "UIName", UINameString, label, sizeof(label));
    std::memcpy(UINameString, label, 31);
    UINameString[31] = 0;
    if (UINameString[0]) {
        store_ui_name(*this, game::scenario_text(UINameString, 8507), true);
        UIName[20] = 0;
    } else {
        store_ui_name(*this, Type->UIName, false);
    }
    RatioAITriggerTeam = ini->ReadInteger(name, "RatioAITriggerTeam", RatioAITriggerTeam);
    RatioTeamAircraft = ini->ReadInteger(name, "RatioTeamAircraft", 75);
    RatioTeamInfantry = ini->ReadInteger(name, "RatioTeamInfantry", 75);
    RatioTeamBuildings = ini->ReadInteger(name, "RatioTeamUnits", 75);
    SideIndex = Type->SideIndex == -1 ? 0 : Type->SideIndex;
    int iq = ini->ReadInteger(name, "IQ", 0);
    if (iq > rules.MaxIQLevels) iq = 1;
    IQLevel = iq;
    IQLevel2 = iq;
    StartingEdge = static_cast<::Edge>(ini->ReadEdge(name, "Edge", -1));
    ColorSchemeIndex = ini->ReadColorString(name, "Color", ColorSchemeIndex);
    if (ColorSchemeIndex < 0) ColorSchemeIndex = 5;
    read_house_colors(*this);
    // The original clears this word before allying the house with itself,
    // which CanAlly refuses.
    AIGeneral = nullptr;
    MakeAlly(Array[ArrayIndex], false);
    const auto allies = static_cast<std::uint32_t>(ini->ReadHousesList(name, "Allies", 0));
    for (int i = 0; i < Array.Count; ++i)
        if (allies & (1u << (unsigned(Array[i]->ArrayIndex) & 31u))) MakeAlly(Array[i], false);
    TeamDelayTimer.Start(175 * ArrayIndex + TeamDelayFor(int(AIDifficulty)));
    Base.LoadFromINI(ini, name);
    Base.Owner = this;
}

void YRPP_FASTCALL HouseClass::LoadFromINIList(CCINIClass* ini) {
    const int count = ini->GetKeyCount("Houses");
    for (int i = 0; i < count; ++i) {
        char name[20];
        ini->ReadString("Houses", ini->GetKeyName("Houses", i), "", name, sizeof(name));
        int country = ini->ReadHouseType(name, "Country", -1);
        // The original indexes the country table with any other result too,
        // including the 4475+n of a "<Player @ X>" name; those also take 0.
        if (country < 0 || country >= HouseTypeClass::Array.Count) country = 0;
        auto* house = new HouseClass(HouseTypeClass::Array[country]);
        std::memcpy(house->PlainName, name, sizeof(name));
        house->PlainName[20] = 0;
    }
    const bool none = Array.Count == 0;
    for (int i = 0; i < Array.Count; ++i) {
        auto* house = Array[i];
        house->LoadFromINI(ini);
        if (campaign()) {
            house->AssignHandicap(house->IsHumanPlayer || house->IsInPlayerControl
                                      ? ScenarioClass::Instance->Difficulty1
                                      : ScenarioClass::Instance->Difficulty2);
        }
    }
    if (none) {
        for (int i = 0; i < HouseTypeClass::Array.Count; ++i) {
            auto* house = new HouseClass(HouseTypeClass::Array[i]);
            house->LoadFromINI(ini);
        }
    }
}
