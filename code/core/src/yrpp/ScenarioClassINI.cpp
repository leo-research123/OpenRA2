/*
 * Scenario INI state follows EA REDALERT/SCENARIO.CPP,
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae (GPL-3.0-or-later;
 * Copyright 2020 Electronic Arts Inc., third_party/ea/LICENSE.TXT).
 * YR fields, defaults and call order calibrated to 689E90..68B891.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>

namespace {
void read_special_flags(ScenarioFlags& flags, INIClass& ini, bool campaign, bool armageddon) {
    // 6B8CA0: multiplayer retains the session's growth/alliance settings.
#define FLAG(name) flags.name = ini.ReadBool("SpecialFlags", #name, flags.name)
    FLAG(TiberiumExplosive); FLAG(MCVDeploy); FLAG(InitialVeteran);
    FLAG(IonStorms); FLAG(Meteorites); FLAG(Visceroids);
    if (campaign || armageddon) {
        FLAG(TiberiumGrows); FLAG(TiberiumSpreads); FLAG(DestroyableBridges);
        FLAG(FixedAlliance); FLAG(FogOfWar); FLAG(Inert); FLAG(HarvesterImmune);
    }
#undef FLAG
}
bool write_special_flags(const ScenarioFlags& flags, INIClass& ini) {
    bool success = true;
#define FLAG(name) success = ini.WriteBool("SpecialFlags", #name, flags.name) && success
    FLAG(TiberiumGrows); FLAG(TiberiumSpreads); FLAG(TiberiumExplosive); FLAG(DestroyableBridges);
    FLAG(MCVDeploy); FLAG(InitialVeteran); FLAG(FixedAlliance); FLAG(HarvesterImmune);
    FLAG(FogOfWar); FLAG(Inert); FLAG(IonStorms); FLAG(Meteorites); FLAG(Visceroids);
#undef FLAG
    return success;
}
Point2D map_pixel(CellClass& cell) {
    // 68AE5C/68AE6C and 68B011/68B021 evaluate Y before X, through two
    // separate virtual calls. No camera offset or Z term is used here.
    CoordStruct coordinates;
    const int y = cell.GetCoords(&coordinates)->Y;
    const int x = cell.GetCoords(&coordinates)->X;
    return TacticalClass::CoordsToMapPixel(x, y);
}
template<class T, class Reader>
void read_list(TypeList<T>& list, INIClass& ini, const char* key, Reader reader) {
    alignas(TypeList<T>) unsigned char storage[sizeof(TypeList<T>)];
    auto* result = reader(reinterpret_cast<TypeList<T>*>(storage), &ini, "Basic", key, list);
    list = std::move(*result);
    result->~TypeList<T>();
}
int truncate_lighting(double value) {
    // 7C5F00 uses x87 truncation to int64, with the caller retaining EAX.
    // An invalid/out-of-int64 conversion produces 80000000:00000000.
    if (!std::isfinite(value) || value < -0x1p63 || value >= 0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
void read_light(INIClass& ini, const char* key, int& value, int scale) {
    // Ground/Level/rate defaults multiply the original single-precision
    // 7F0E78 constant. Using a double 0.001 changes large saved defaults.
    const double inverse = scale == 100 ? 0.01 : static_cast<double>(0.001f);
    value = truncate_lighting(ini.ReadDouble("Lighting", key, value * inverse) * scale + 0.01);
}
}

bool ScenarioClass::ReadINI(INIClass& ini) {
    const auto& runtime = game::scenario_runtime();
    const auto* loading = runtime.ini;
    const auto* world = runtime.houses;
    if (!loading || !loading->armageddon_mode) return false;
    if (!loading->fields_only && (!loading->progress || !loading->pump_events ||
        !loading->reset_lighting || !world || !world->houses || !world->current_player)) return false;
    const bool campaign = runtime.session_mode(runtime.context) == 0;
    ini.CurrentSectionName = nullptr;
    ini.CurrentSection = nullptr;
    read_special_flags(SpecialFlags, ini, campaign, *loading->armageddon_mode);

    ini.GetInteger("Header", "StartX", StartX);
    ini.GetInteger("Header", "StartY", StartY);
    ini.GetInteger("Header", "Width", Width);
    ini.GetInteger("Header", "Height", Height);
    ini.GetInteger("Header", "NumberStartingPoints", NumberStartingPoints);
    ini.GetInteger("Header", "NumCoopHumanStartSpots", NumCoopHumanStartSpots);
    for (int i = 0; i < std::min(NumberStartingPoints, 8); ++i) {
        char key[16]; std::snprintf(key, sizeof(key), "Waypoint%d", i + 1);
        ini.GetPoint2D("Header", key, StartingPoints[i]);
    }
    ini.GetString("Basic", "NextScenario", NextScenario);
    ini.GetString("Basic", "AltNextScenario", AltNextScenario);
#define MOVIE(name) name = ini.ReadMovie("Basic", #name, name)
    MOVIE(Intro); MOVIE(Brief); MOVIE(Win); MOVIE(Lose); MOVIE(Action);
    MOVIE(PostScore); MOVIE(PreMapSelect);
#undef MOVIE
#define BOOL(name) ini.GetBool("Basic", #name, name)
    BOOL(MultiplayerOnly); BOOL(TimerInherit); BOOL(EndOfGame);
    ThemeIndex = ini.ReadTheme("Basic", "Theme", ThemeIndex);
    NewINIFormat = ini.ReadInteger("Basic", "NewINIFormat", 0);
    CarryOverMoney = ini.ReadDouble("Basic", "CarryOverMoney", CarryOverMoney);
    if (CarryOverMoney >= 1.0) CarryOverMoney = 1.0;
    ini.GetInteger("Basic", "CarryOverCap", CarryOverCap);
    BOOL(SkipScore); BOOL(OneTimeOnly); BOOL(SkipMapSelect); BOOL(TruckCrate);
    BOOL(TrainCrate); BOOL(FillSilos); BOOL(IgnoreGlobalAITriggers);
    ini.GetInteger("Basic", "Percent", Percent);
    ini.GetInteger("Basic", "StartingDropships", StartingDropships);
    read_list(AllowableUnits, ini, "AllowableUnits", INIClass::GetTechnoTypes);
    read_list(AllowableUnitMaximums, ini, "AllowableUnitMaximums", INIClass::GetIntegers);
    BOOL(TiberiumGrowthEnabled); BOOL(VeinGrowthEnabled); BOOL(IceGrowthEnabled);
    BOOL(TiberiumDeathToVisceroid); BOOL(FreeRadar);
#undef BOOL
    ini.GetInteger("Basic", "HomeCell", HomeCell);
    ini.GetInteger("Basic", "AltHomeCell", AltHomeCell);
    ini.ReadBool("Basic", "CivEvac", false); // Read and discard in the target.
    // The target recomputes the shrinking deficit after every append. Do not
    // replace this with a while loop that fills the entire deficit.
    for (int i = 0; i < AllowableUnits.Count - AllowableUnitMaximums.Count; ++i)
        if (!AllowableUnitMaximums.AddItem(-1)) return false;
    // Repeated reads append another set; the startup reset owns clearing it.
    for (int i = 0; i < AllowableUnitMaximums.Count; ++i)
        if (!DropshipUnitCounts.AddItem(0)) return false;
    if (!ReadLocalVariables(ini)) return false;
    if (!loading->fields_only) loading->pump_events(loading->context);
    ParTimeEasy = ini.ReadTime("Ranking", "ParTimeEasy", ParTimeEasy);
    ParTimeMedium = ini.ReadTime("Ranking", "ParTimeMedium", ParTimeMedium);
    ParTimeDifficult = ini.ReadTime("Ranking", "ParTimeHard", ParTimeDifficult);
    ini.ReadString("Ranking", "UnderParTitle", "GUI:NoUnderParTitle", UnderParTitle);
    ini.ReadString("Ranking", "UnderParMessage", "GUI:NoUnderParMessage", UnderParMessage);
    ini.ReadString("Ranking", "OverParTitle", "GUI:NoOverParTitle", OverParTitle);
    ini.ReadString("Ranking", "OverParMessage", "GUI:NoOverParMessage", OverParMessage);
    if (!ReadLightingINI(ini)) return false;
    if (loading->fields_only) return true;
    loading->progress(loading->context, 55);
    loading->pump_events(loading->context);
    // The event pump may update the session; the original tests its live mode
    // again here, independently of the earlier SpecialFlags decision.
    if (runtime.session_mode(runtime.context) == 0) {
        char player[20]; ini.ReadString("Basic", "Player", "", player);
        int index = 0;
        for (int i = 0; i < world->houses->Count; ++i) {
            const auto* house = world->houses->Items[i];
            if (house && !std::strcmp(house->PlainName, player)) { index = i; break; }
        }
        if (!world->houses->ValidIndex(index) || !world->houses->Items[index] ||
            !world->houses->Items[index]->Type) return false;
        *world->current_player = world->houses->Items[index];
        HumanPlayerHouseTypeIndex = (*world->current_player)->Type->ArrayIndex2;
    } else AssignHouses();
    auto* player = *world->current_player;
    if (!player) return false;
    player->IsHumanPlayer = true;
    player->IsInPlayerControl = true;
    player->CurrentDropshipIndex = 0;
    loading->progress(loading->context, 58);
    loading->pump_events(loading->context);
    if (*loading->armageddon_mode) loading->reset_lighting(loading->context);
    loading->progress(loading->context, 60);
    loading->pump_events(loading->context);
    return true;
}

bool ScenarioClass::WriteINI(INIClass& ini, bool multiplayer) {
    const auto& runtime = game::scenario_runtime();
    const auto* render = runtime.render;
    const auto* world = runtime.houses;
    if (!Instance || !render || !render->map || !runtime.cell_at ||
        (!multiplayer && (!world || !world->current_player || !*world->current_player))) return false;
    bool success = true;
    ini.Clear("Basic");
    ini.Clear("Basic", "CivEvac");
    if (Instance->BriefingCSF[0]) success = ini.WriteString("Basic", "Briefing", Instance->BriefingCSF) && success;
    if (Instance->UIName[0]) success = ini.WriteString("Basic", "UIName", Instance->UIName) && success;
    auto& map = *render->map;
    map.CellIteratorReset();
    int min_x = 10000, min_y = 10000, max_x = 0, max_y = 0;
    while (auto* cell = map.CellIteratorNext()) {
        if (!map.IsWithinUsableArea2D(cell->MapCoords)) continue;
        const auto pixel = map_pixel(*cell);
        const int x = pixel.X / 60, y = pixel.Y / 30;
        min_x = std::min(min_x, x); min_y = std::min(min_y, y);
        max_x = std::max(max_x, x); max_y = std::max(max_y, y);
    }
    success = ini.WriteInteger("Header", "StartX", min_x) && success;
    success = ini.WriteInteger("Header", "StartY", min_y) && success;
    success = ini.WriteInteger("Header", "Width", max_x - min_x) && success;
    success = ini.WriteInteger("Header", "Height", max_y - min_y) && success;
    int count = 0;
    for (int i = 0; i < 8; ++i) if (Instance->IsDefinedWaypoint(i)) ++count;
    success = ini.WriteInteger("Header", "NumberStartingPoints", count) && success;
    success = ini.WriteInteger("Header", "NumCoopHumanStartSpots", NumCoopHumanStartSpots) && success;
    for (int i = 0; i < 8; ++i) {
        char key[16]; std::snprintf(key, sizeof(key), "Waypoint%d", i + 1);
        int position[2]{};
        // The count is based on all eight slots, but export consumes the first
        // count slots, including any holes. Preserve the original ordering.
        if (i < count) {
            CellClass* cell = nullptr;
            if (!runtime.cell_at(runtime.context, Instance->Waypoints[i], cell) || !cell) return false;
            const auto pixel = map_pixel(*cell);
            position[0] = pixel.X / 60; position[1] = pixel.Y / 30;
        }
        success = ini.Write2Integers("Header", key, position) && success;
    }
    success = write_special_flags(SpecialFlags, ini) && success;
    success = ini.WriteString("Basic", "NextScenario", NextScenario) && success;
    success = ini.WriteString("Basic", "AltNextScenario", AltNextScenario) && success;
    success = ini.WriteUnicodeString("Basic", "Name", Name) && success;
    success = ini.WriteInteger("Basic", "NewINIFormat", 4) && success;
    success = ini.WriteInteger("Basic", "CarryOverCap", CarryOverCap / 100) && success;
#define BOOL(name) success = ini.WriteBool("Basic", #name, name) && success
    BOOL(EndOfGame); BOOL(SkipScore); BOOL(OneTimeOnly); BOOL(SkipMapSelect);
    success = ini.WriteBool("Basic", "Official", true) && success;
    BOOL(IgnoreGlobalAITriggers); BOOL(TruckCrate); BOOL(TrainCrate);
    success = ini.WriteInteger("Basic", "Percent", Percent) && success;
    success = ini.WriteTechnoTypes("Basic", "AllowableUnits", AllowableUnits) && success;
    success = ini.WriteIntegers("Basic", "AllowableUnitMaximums", AllowableUnitMaximums) && success;
    success = ini.WriteTheme("Basic", "Theme", ThemeIndex) && success;
    if (!multiplayer) {
        success = ini.WriteString("Basic", "Player", (*world->current_player)->PlainName) && success;
#define MOVIE(name) success = ini.WriteMovie("Basic", #name, name) && success
        MOVIE(Intro); MOVIE(Brief); MOVIE(Win); MOVIE(Lose); MOVIE(Action);
        MOVIE(PostScore); MOVIE(PreMapSelect);
#undef MOVIE
        success = ini.WriteDouble("Basic", "CarryOverMoney", CarryOverMoney) && success;
        BOOL(TimerInherit); BOOL(FillSilos);
        success = ini.WriteInteger("Basic", "StartingDropships", StartingDropships) && success;
        success = ini.WriteInteger("Basic", "HomeCell", HomeCell) && success;
        success = ini.WriteInteger("Basic", "AltHomeCell", AltHomeCell) && success;
    }
    success = ini.WriteInteger("Basic", "MultiplayerOnly", MultiplayerOnly) && success;
    BOOL(TiberiumGrowthEnabled); BOOL(VeinGrowthEnabled); BOOL(IceGrowthEnabled);
    BOOL(TiberiumDeathToVisceroid); BOOL(FreeRadar);
#undef BOOL
    success = ini.WriteInteger("Basic", "InitTime", InitTime) && success;
    success = WriteLocalVariables(ini) && success;
    success = ini.WriteTime("Ranking", "ParTimeEasy", ParTimeEasy) && success;
    success = ini.WriteTime("Ranking", "ParTimeMedium", ParTimeMedium) && success;
    success = ini.WriteTime("Ranking", "ParTimeHard", ParTimeDifficult) && success;
    success = ini.WriteString("Ranking", "UnderParTitle", UnderParTitle) && success;
    success = ini.WriteString("Ranking", "UnderParMessage", UnderParMessage) && success;
    success = ini.WriteString("Ranking", "OverParTitle", OverParTitle) && success;
    success = ini.WriteString("Ranking", "OverParMessage", OverParMessage) && success;
    ini.Clear("Lighting");
    const auto light = [&](const char* key, int value, double inverse) {
        success = ini.WriteDouble("Lighting", key, value * inverse) && success;
    };
    light("Ambient", AmbientOriginal, 0.01);
    light("Red", NormalLighting.Tint.Red, 0.01);
    light("Green", NormalLighting.Tint.Green, 0.01);
    light("Blue", NormalLighting.Tint.Blue, 0.01);
    light("Ground", NormalLighting.Ground, 0.001);
    light("Level", NormalLighting.Level, 0.001);
    light("IonAmbient", IonAmbient, 0.01);
    light("IonRed", IonLighting.Tint.Red, 0.01);
    light("IonGreen", IonLighting.Tint.Green, 0.01);
    light("IonBlue", IonLighting.Tint.Blue, 0.01);
    light("IonGround", IonLighting.Ground, 0.001);
    light("IonLevel", IonLighting.Level, 0.001);
    light("DominatorAmbient", DominatorAmbient, 0.01);
    light("DominatorRed", DominatorLighting.Tint.Red, 0.01);
    light("DominatorGreen", DominatorLighting.Tint.Green, 0.01);
    light("DominatorBlue", DominatorLighting.Tint.Blue, 0.01);
    light("DominatorGround", DominatorLighting.Ground, 0.001);
    light("DominatorLevel", DominatorLighting.Level, 0.001);
    light("DominatorAmbientChangeRate", DominatorAmbientChangeRate, 0.001);
    return success;
}

bool ScenarioClass::ReadLightingINI(INIClass& ini) noexcept {
    try {
    read_light(ini, "Ambient", AmbientOriginal, 100);
    AmbientCurrent = AmbientTarget = AmbientOriginal;
    read_light(ini, "Red", NormalLighting.Tint.Red, 100);
    read_light(ini, "Green", NormalLighting.Tint.Green, 100);
    read_light(ini, "Blue", NormalLighting.Tint.Blue, 100);
    read_light(ini, "Ground", NormalLighting.Ground, 1000);
    read_light(ini, "Level", NormalLighting.Level, 1000);
    read_light(ini, "IonAmbient", IonAmbient, 100);
    read_light(ini, "IonRed", IonLighting.Tint.Red, 100);
    read_light(ini, "IonGreen", IonLighting.Tint.Green, 100);
    read_light(ini, "IonBlue", IonLighting.Tint.Blue, 100);
    read_light(ini, "IonGround", IonLighting.Ground, 1000);
    read_light(ini, "IonLevel", IonLighting.Level, 1000);
    NukeAmbientChangeRate = truncate_lighting(ini.ReadDouble("Lighting", "NukeAmbientChangeRate", NukeAmbientChangeRate));
    read_light(ini, "DominatorAmbient", DominatorAmbient, 100);
    read_light(ini, "DominatorRed", DominatorLighting.Tint.Red, 100);
    read_light(ini, "DominatorGreen", DominatorLighting.Tint.Green, 100);
    read_light(ini, "DominatorBlue", DominatorLighting.Tint.Blue, 100);
    read_light(ini, "DominatorGround", DominatorLighting.Ground, 1000);
    read_light(ini, "DominatorLevel", DominatorLighting.Level, 1000);
    read_light(ini, "DominatorAmbientChangeRate", DominatorAmbientChangeRate, 1000);
        return true;
    } catch (...) { return false; }
}
