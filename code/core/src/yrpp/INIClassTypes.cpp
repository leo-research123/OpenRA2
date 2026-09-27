// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from pinned EA WWLib INIClass::Get_List_Index/Get_Int_Bitfield.
// YR names, numeric mappings, defaults and lookup order calibrated from fixed
// gamemd 4748A0..477640 and their callees. Tables retain original spellings.
#include "yrpp/CCINIClass.h"
#include "yrpp/Theater.h"
#include "yrpp/TechnoTypeClass.h"
#include "ini_runtime.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <new>
#include <stdexcept>
#include <utility>

namespace {
struct NameValue { const char* name; int value; };
constexpr NameValue pip[] = {
    {"green", 1},
    {"yellow", 2},
    {"white", 3},
    {"red", 4},
    {"blue", 5},
    {"persongreen", 7},
    {"personyellow", 8},
    {"personwhite", 9},
    {"personred", 10},
    {"personblue", 11},
    {"personpurple", 12},
};
constexpr NameValue pip_scale[] = {
    {"Ammo", 1},
    {"Tiberium", 2},
    {"Passengers", 3},
    {"Power", 4},
    {"MindControl", 5},
};
constexpr NameValue foundation[] = {
    {"1x1", 0},
    {"2x1", 1},
    {"1x2", 2},
    {"2x2", 3},
    {"2x3", 4},
    {"3x2", 5},
    {"3x3", 6},
    {"3x5", 7},
    {"4x2", 8},
    {"3x3Refinery", 9},
    {"1x3", 10},
    {"3x1", 11},
    {"4x3", 12},
    {"1x4", 13},
    {"1x5", 14},
    {"2x6", 15},
    {"2x5", 16},
    {"5x3", 17},
    {"4x4", 18},
    {"3x4", 19},
    {"6x4", 20},
    {"0x0", 21},
};
constexpr NameValue movement_zone[] = {
    {"Normal", 0},
    {"Crusher", 1},
    {"Destroyer", 2},
    {"AmphibiousDestroyer", 3},
    {"AmphibiousCrusher", 4},
    {"Amphibious", 5},
    {"Subterannean", 6},
    {"Infantry", 7},
    {"InfantryDestroyer", 8},
    {"Fly", 9},
    {"Water", 10},
    {"WaterBeach", 11},
    {"CrusherAll", 12},
};
constexpr NameValue action[] = {
    {"None", 0},
    {"Move", 1},
    {"NoMove", 2},
    {"Enter", 3},
    {"Self", 4},
    {"Attack", 5},
    {"Harvest", 6},
    {"Select", 7},
    {"ToggleSelect", 8},
    {"Capture", 9},
    {"Eaten", 10},
    {"Repair", 11},
    {"Sell", 12},
    {"SellUnit", 13},
    {"NoSell", 14},
    {"NoRepair", 15},
    {"Sabotage", 16},
    {"Tote", 17},
    {"DontUse2", 18},
    {"DontUse3", 19},
    {"Nuke", 20},
    {"DontUse4", 21},
    {"DontUse5", 22},
    {"DontUse6", 23},
    {"DontUse7", 24},
    {"DontUse8", 25},
    {"GuardArea", 26},
    {"Heal", 27},
    {"Damage", 28},
    {"GRepair", 29},
    {"NoDeploy", 30},
    {"NoEnter", 31},
    {"NoGRepair", 32},
    {"TogglePower", 33},
    {"NoTogglePower", 34},
    {"EnterTunnel", 35},
    {"NoEnterTunnel", 36},
    {"IronCurtain", 37},
    {"LightningStorm", 38},
    {"ChronoSphere", 39},
    {"ChronoWarp", 40},
    {"ParaDrop", 41},
    {"PlaceWaypoint", 42},
    {"TibSunBug", 43},
    {"EnterWaypointMode", 44},
    {"FollowWaypoint", 45},
    {"SelectWaypoint", 46},
    {"LoopWaypointPath", 47},
    {"DragWaypoint", 48},
    {"AttackWaypoint", 49},
    {"EnterWaypoint", 50},
    {"PatrolWaypoint", 51},
    {"AreaAttack", 52},
    {"IvanBomb", 53},
    {"NoIvanBomb", 54},
    {"Detonate", 55},
    {"DetonateAll", 56},
    {"DisarmBomb", 57},
    {"SelectNode", 58},
    {"AttackSupport", 59},
    {"PlaceBeacon", 60},
    {"SelectBeacon", 61},
    {"AttackMoveNav", 62},
    {"AttackMoveTar", 63},
    {"Demolish", 64},
    {"AmerParaDrop", 65},
    {"PsychicDominator", 66},
    {"SpyPlane", 67},
    {"GeneticConverter", 68},
    {"ForceShield", 69},
    {"NoForceShield", 70},
    {"Airstrike", 71},
    {"PsychicReveal", 72},
};
constexpr NameValue vhp[] = {
    {"None", 0},
    {"Normal", 1},
    {"Strong", 2},
};
constexpr NameValue armor[] = {
    {"none", 0},
    {"flak", 1},
    {"plate", 2},
    {"light", 3},
    {"medium", 4},
    {"heavy", 5},
    {"wood", 6},
    {"steel", 7},
    {"concrete", 8},
    {"special_1", 9},
    {"special_2", 10},
};
constexpr NameValue category[] = {
    {"Soldier", 0},
    {"Civilian", 1},
    {"VIP/Agent", 2},
    {"Recon Vehicle", 3},
    {"Armored Fighting Vehicle", 4},
    {"Infantry Fighting Vehicle", 5},
    {"Indirect Fire Support", 6},
    {"Misc. Support Vehicle", 7},
    {"Transport Vehicle", 8},
    {"Air Combat Support", 9},
    {"Air Transport", 10},
};
constexpr NameValue factory[] = {
    {"<none>", 0},
    {"Unit", 1},
    {"Aircraft", 2},
    {"AircraftType", 3},
    {"Anim", 4},
    {"AnimType", 5},
    {"Building", 6},
    {"BuildingType", 7},
    {"Bullet", 8},
    {"BulletType", 9},
    {"Campaign", 10},
    {"Cell", 11},
    {"Factory", 12},
    {"House", 13},
    {"HouseType", 14},
    {"Infantry", 15},
    {"InfantryType", 16},
    {"Isotile", 17},
    {"IsotileType", 18},
    {"Light", 19},
    {"Overlay", 20},
    {"OverlayType", 21},
    {"Particle", 22},
    {"ParticleType", 23},
    {"ParticleSystem", 24},
    {"ParticleSystemType", 25},
    {"Script", 26},
    {"ScriptType", 27},
    {"Side", 28},
    {"Smudge", 29},
    {"SmudgeType", 30},
    {"Special", 31},
    {"SuperWeaponType", 32},
    {"TaskForce", 33},
    {"Team", 34},
    {"TeamType", 35},
    {"Terrain", 36},
    {"TerrainType", 37},
    {"Trigger", 38},
    {"TriggerType", 39},
    {"UnitType", 40},
    {"VoxelAnim", 41},
    {"VoxelAnimType", 42},
    {"Wave", 43},
    {"Tag", 44},
    {"TagType", 45},
    {"Tiberium", 46},
    {"Action", 47},
    {"Event", 48},
    {"WeaponType", 49},
    {"WarheadType", 50},
    {"Waypoint", 51},
    {"Abstract", 52},
    {"Tube", 53},
    {"LightSource", 54},
    {"EMPulse", 55},
    {"TacticalMap", 56},
    {"SuperWeapon", 57},
    {"AITrigger", 58},
    {"AITriggerType", 59},
    {"Neuron", 60},
    {"FoggedObject", 61},
    {"AlphaShape", 62},
    {"VeinholeMonster", 63},
    {"NavyType", 64},
    {"SpawnManager", 65},
    {"CaptureManager", 66},
    {"Parasite", 67},
    {"Bomb", 68},
    {"Temporal", 70},
    {"RadSite", 69},
    {"Airstrike", 71},
    {"SlaveManager", 72},
    {"DiskLaser", 73},
};
constexpr NameValue build_cat[] = {
    {"DontCare", 0},
    {"Tech", 1},
    {"Power", 3},
    {"Resource", 2},
    {"Infrastructure", 4},
    {"Combat", 5},
};
constexpr NameValue land[] = {
    {"Clear", 0},
    {"Road", 1},
    {"Water", 2},
    {"Rock", 3},
    {"Wall", 4},
    {"Tiberium", 5},
    {"Beach", 6},
    {"Rough", 7},
    {"Ice", 8},
    {"Railroad", 9},
    {"Tunnel", 10},
    {"Weeds", 11},
};
constexpr NameValue speed[] = {
    {"Foot", 0},
    {"Track", 1},
    {"Wheel", 2},
    {"Hover", 3},
    {"Winged", 4},
    {"Float", 5},
    {"Amphibious", 6},
    {"FloatBeach", 7},
};
constexpr NameValue layer[] = {
    {"Underground", 0},
    {"Surface", 1},
    {"Ground", 2},
    {"Air", 3},
    {"Top", 4},
};
constexpr NameValue powerup[] = {
    {"Money", 0},
    {"Unit", 1},
    {"HealBase", 2},
    {"Cloak", 3},
    {"Explosion", 4},
    {"Napalm", 5},
    {"Squad", 6},
    {"Darkness", 7},
    {"Reveal", 8},
    {"Armor", 9},
    {"Speed", 10},
    {"Firepower", 11},
    {"ICBM", 12},
    {"Invulnerability", 13},
    {"Veteran", 14},
    {"IonStorm", 15},
    {"Gas", 16},
    {"Tiberium", 17},
    {"Pod", 18},
};
constexpr NameValue edge[] = {
    {"North", 0},
    {"East", 1},
    {"South", 2},
    {"West", 3},
    {"Air", 4},
};
constexpr NameValue ability[] = {
    {"FASTER", 0},
    {"STRONGER", 1},
    {"FIREPOWER", 2},
    {"SCATTER", 3},
    {"ROF", 4},
    {"SIGHT", 5},
    {"CLOAK", 6},
    {"TIBERIUM_PROOF", 7},
    {"VEIN_PROOF", 8},
    {"SELF_HEAL", 9},
    {"EXPLODES", 10},
    {"RADAR_INVISIBLE", 11},
    {"SENSORS", 12},
    {"FEARLESS", 13},
    {"C4", 14},
    {"TIBERIUM_HEAL", 15},
    {"GUARD_AREA", 16},
    {"CRUSHER", 17},
};
constexpr NameValue players[] = {
    {"<Player @ A>", 4475},
    {"<Player @ B>", 4476},
    {"<Player @ C>", 4477},
    {"<Player @ D>", 4478},
    {"<Player @ E>", 4479},
    {"<Player @ F>", 4480},
    {"<Player @ G>", 4481},
    {"<Player @ H>", 4482},
};
constexpr NameValue category_alias[] = {
    {"Soldier", 0},
    {"Civilian", 1},
    {"VIP", 2},
    {"Recon", 3},
    {"AFV", 4},
    {"IFV", 5},
    {"LRFS", 6},
    {"Support", 7},
    {"Transport", 8},
    {"AirPower", 9},
    {"AirLift", 10},
};
bool equal_name(const char* a, const char* b) {
    if (!a || !b) return false;
    for (;; ++a, ++b) {
        const auto fold = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
        if (fold(*a) != fold(*b)) return false;
        if (!*a) return true;
    }
}
template<size_t N>
int find_name(const NameValue (&table)[N], const char* text, int missing) {
    for (const auto& entry : table) if (equal_name(entry.name, text)) return entry.value;
    return missing;
}
template<size_t N>
const char* indexed_name(const NameValue (&table)[N], int index) {
    return index >= 0 && size_t(index) < N ? table[index].name : "<none>";
}
template<size_t N>
const char* value_name(const NameValue (&table)[N], int value) {
    for (const auto& entry : table) if (entry.value == value) return entry.name;
    return table[0].name;
}
template<size_t N, size_t Capacity>
int read_named(INIClass& ini, const char* section, const char* key, const char* fallback,
        const NameValue (&table)[N], int missing, char (&text)[Capacity]) {
    ini.ReadString(section, key, fallback, text, Capacity);
    return find_name(table, text, missing);
}
template<class Operation>
void each_token(char* text, Operation operation) {
    auto* token = text;
    while (*token) {
        token += std::strspn(token, ",");
        if (!*token) break;
        auto* end = token + std::strcspn(token, ",");
        const bool more = *end != 0;
        *end = 0;
        operation(token);
        if (!more) break;
        token = end + 1;
    }
}
using Kind = game::IniTypeKind;
game::IniTypeResult find_type(Kind kind, const char* text, bool allocate = false) {
    const auto& runtime = game::ini_runtime();
    game::IniTypeResult result;
    if (!runtime.find(runtime.context, kind, text, allocate, result)) return {};
    return result;
}
const char* type_name(Kind kind, int index) {
    if (index < 0) return "";
    const auto& runtime = game::ini_runtime();
    const char* name = nullptr;
    return runtime.name(runtime.context, kind, index, name) && name ? name : "";
}
}

int INIClass::ReadPip(const char* section, const char* key, int fallback) {
    char text[32]{};
    // The target indexes the fallback name table, then scans from its beginning.
    return read_named(*this, section, key, indexed_name(pip, fallback), pip, 1, text);
}
int INIClass::ReadPipScale(const char* section, const char* key, int fallback) {
    char text[32]{};
    return ReadString(section, key, "", text, sizeof(text)) ? find_name(pip_scale, text, 0) : fallback;
}
int INIClass::ReadCategory(const char* section, const char* key, int fallback) {
    char text[32]{};
    ReadString(section, key, indexed_name(category_alias, fallback), text, sizeof(text));
    return find_name(category, text, find_name(category_alias, text, -1));
}
int INIClass::ReadFoundation(const char* section, const char* key, int fallback) {
    char text[32]{};
    return read_named(*this, section, key, indexed_name(foundation, fallback), foundation, 0, text);
}
int INIClass::ReadMovementZone(const char* section, const char* key, int fallback) {
    char text[32]{};
    return read_named(*this, section, key, indexed_name(movement_zone, fallback), movement_zone, -1, text);
}
int INIClass::ReadSWAction(const char* section, const char* key, int fallback) {
    char text[32]{};
    return read_named(*this, section, key, indexed_name(action, fallback), action, 0, text);
}
int INIClass::ReadFactory(const char* section, const char* key, int fallback) {
    char text[32]{};
    return read_named(*this, section, key, value_name(factory, fallback), factory, 0, text);
}
int INIClass::ReadBuildCat(const char* section, const char* key, int fallback) {
    char text[32]{};
    return read_named(*this, section, key, value_name(build_cat, fallback), build_cat, 0, text);
}
int INIClass::ReadArmorType(const char* section, const char* key, int fallback) {
    char text[128]{};
    return read_named(*this, section, key, indexed_name(armor, fallback), armor, 0, text);
}
#define READ_INDEXED(Name, Table) \
int INIClass::Read##Name(const char* section, const char* key, int fallback) { \
    char text[128]{}; \
    return ReadString(section, key, indexed_name(Table, fallback), text, sizeof(text)) \
        ? find_name(Table, text, -1) : fallback; \
}
READ_INDEXED(LandType, land)
READ_INDEXED(SpeedType, speed)
READ_INDEXED(Layer, layer)
#undef READ_INDEXED
int INIClass::ReadTheater(const char* section, const char* key, int fallback) {
    char text[128]{};
    if (!ReadString(section, key, "", text, sizeof(text))) return fallback;
    for (int i = 0; i < 6; ++i) if (equal_name(Theater::Array[i].ID, text)) return i;
    return -1;
}
int INIClass::ReadEdge(const char* section, const char* key, int fallback) {
    char text[128]{};
    return ReadString(section, key, "", text, sizeof(text)) ? find_name(edge, text, -1) : fallback;
}
int INIClass::ReadPowerup(const char* section, const char* key, int fallback) {
    char text[128]{};
    return ReadString(section, key, "", text, sizeof(text)) ? find_name(powerup, text, 0) : fallback;
}
int INIClass::ReadVHPScan(const char* section, const char* key, int fallback) {
    char text[128]{};
    ReadString(section, key, indexed_name(vhp, fallback), text, sizeof(text));
    auto* token = text + std::strspn(text, ",");
    token[std::strcspn(token, ",")] = 0;
    return find_name(vhp, token, fallback);
}
byte* INIClass::ReadAbilities(byte* output, const char* section, const char* key, byte* fallback) {
    if (!output) return nullptr;
    char text[128]{};
    byte values[18]{};
    if (ReadString(section, key, "", text, sizeof(text))) {
        each_token(text, [&](const char* token) {
            const int index = find_name(ability, token, -1);
            if (index >= 0) values[index] = 1;
        });
    } else if (fallback) std::copy_n(fallback, 18, values);
    std::copy_n(values, 18, output);
    return output;
}

int INIClass::ReadColorString(const char* section, const char* key, int fallback) {
    char text[32]{};
    ReadString(section, key, type_name(Kind::color_scheme, fallback), text, sizeof(text));
    const int index = find_type(Kind::color_scheme, text).index;
    return index == -1 ? fallback : index;
}
int INIClass::ReadSWType(const char* section, const char* key, int fallback) {
    char text[32]{};
    ReadString(section, key, type_name(Kind::super_weapon, fallback), text, sizeof(text));
    return find_type(Kind::super_weapon, text).index;
}
int INIClass::ReadVoxName(const char* section, const char* key, int fallback) {
    char text[100]{};
    return ReadString(section, key, "", text, sizeof(text)) ? find_type(Kind::vox, text).index : fallback;
}
int INIClass::ReadHouseTypesList(const char* section, const char* key, int fallback) {
    char text[128]{};
    if (!ReadString(section, key, "", text, sizeof(text))) return fallback;
    std::uint32_t bits = 0;
    each_token(text, [&](const char* token) {
        const int index = find_type(Kind::house_type, token).index;
        if (index != -1) bits |= 1u << (std::uint32_t(index) & 31u);
    });
    return std::bit_cast<std::int32_t>(bits);
}
int INIClass::ReadHousesList(const char* section, const char* key, int fallback) {
    char text[128]{};
    if (!ReadString(section, key, "", text, sizeof(text))) return fallback;
    std::uint32_t bits = 0;
    each_token(text, [&](const char* token) {
        // Target SHL masks the shift count: a missing house (-1) sets bit 31.
        const int index = find_type(Kind::house, token).index;
        bits |= 1u << (std::uint32_t(index) & 31u);
    });
    return std::bit_cast<std::int32_t>(bits);
}
int INIClass::ReadHouseType(const char* section, const char* key, int fallback) {
    char text[128]{};
    if (!ReadString(section, key, "", text, sizeof(text))) return fallback;
    for (int i = 0; i < 8; ++i) if (!std::strcmp(text, players[i].name)) return 4475 + i;
    return find_type(Kind::house_type, text, true).index;
}
int INIClass::ReadSide(const char* section, const char* key, int fallback) {
    char text[128]{};
    return ReadString(section, key, "", text, sizeof(text)) ? find_type(Kind::side, text, true).index : fallback;
}
int INIClass::ReadMovie(const char* section, const char* key, int fallback) {
    char text[128]{};
    if (!ReadString(section, key, "", text, sizeof(text))) return fallback;
    const int index = find_type(Kind::movie, text).index;
    return index == -1 ? fallback : index;
}
int INIClass::ReadTheme(const char* section, const char* key, int fallback) {
    char text[128]{};
    return ReadString(section, key, "", text, sizeof(text)) ? find_type(Kind::theme, text).index : fallback;
}
TechnoTypeClass* INIClass::GetTechnoType(const char* section, const char* key) {
    char text[128]{};
    ReadString(section, key, "<none>", text, sizeof(text));
    for (const auto kind : {Kind::infantry, Kind::unit, Kind::aircraft, Kind::building}) {
        const auto found = find_type(kind, text);
        if (found.index != -1) return found.techno;
    }
    return nullptr;
}
TypeList<int>* YRPP_FASTCALL INIClass::GetPrerequisites(TypeList<int>* output, INIClass* ini,
        const char* section, const char* key, TypeList<int> fallback) {
    if (!output) return nullptr;
    char text[128]{};
    if (!ini || !ini->ReadString(section, key, "", text, sizeof(text))) {
        // Target 4777B0 only constructs the DynamicVector base; it leaves the
        // extra TypeList field untouched. Initialize it instead of exposing
        // indeterminate return-storage bytes (and do not inherit Defaults').
        TypeList<int> result;
        static_cast<DynamicVectorClass<int>&>(result) = static_cast<const DynamicVectorClass<int>&>(fallback);
        return new (output) TypeList<int>(std::move(result));
    }
    TypeList<int> parsed;
    constexpr NameValue generic[] = {{"POWER", -1}, {"FACTORY", -2}, {"BARRACKS", -3},
        {"RADAR", -4}, {"TECH", -5}, {"PROC", -6}};
    each_token(text, [&](const char* token) {
        int index = find_name(generic, token, 0);
        if (!index) {
            index = find_type(Kind::building, token).index;
            if (index == -1) return;
        }
        if (!parsed.AddItem(index)) throw std::bad_alloc();
    });
    return new (output) TypeList<int>(parsed);
}

// WWLib list readers, calibrated to 475D70/4764F0. Empty input copies the
// DynamicVector defaults. Tokens use comma-only splitting; unknown techno
// names are skipped, whereas malformed integers become zero through atoi.
template<class T, class Parse>
static TypeList<T>* read_type_list(TypeList<T>* output, INIClass* ini,
        const char* section, const char* key, const TypeList<T>& defaults, Parse parse) {
    if (!output) return nullptr;
    TypeList<T> result;
    char text[512]{};
    if (!ini || !ini->ReadString(section, key, "", text, sizeof(text))) {
        static_cast<DynamicVectorClass<T>&>(result) = static_cast<const DynamicVectorClass<T>&>(defaults);
    } else {
        each_token(text, [&](const char* token) {
            T value{};
            if (parse(token, value) && !result.AddItem(value)) throw std::bad_alloc();
        });
    }
    // Original leaves unknown_18 uninitialized in the return storage.
    return new (output) TypeList<T>(std::move(result));
}
TypeList<int>* YRPP_FASTCALL INIClass::GetIntegers(TypeList<int>* output, INIClass* ini,
        const char* section, const char* key, TypeList<int> defaults) {
    return read_type_list(output, ini, section, key, defaults,
        [](const char* text, int& value) { value = std::atoi(text); return true; });
}
TypeList<TechnoTypeClass*>* YRPP_FASTCALL INIClass::GetTechnoTypes(TypeList<TechnoTypeClass*>* output,
        INIClass* ini, const char* section, const char* key, TypeList<TechnoTypeClass*> defaults) {
    return read_type_list(output, ini, section, key, defaults,
        [](const char* text, TechnoTypeClass*& value) {
            const auto found = find_type(Kind::techno, text);
            if (found.index == -1 || !found.techno) return false;
            value = found.techno;
            return true;
        });
}

namespace {
bool append_list_value(char (&text)[512], const char* value) {
    if (!value) return false;
    const auto used = std::strlen(text), size = std::strlen(value);
    const auto separator = used ? 1u : 0u;
    if (used + separator + size >= sizeof(text)) return false;
    if (separator) text[used] = ',';
    std::memcpy(text + used + separator, value, size + 1);
    return true;
}
}
bool INIClass::WriteIntegers(const char* section, const char* key, const TypeList<int>& values) {
    char text[512]{};
    for (int i = 0; i < values.Count; ++i) {
        char item[12]; std::snprintf(item, sizeof(item), "%d", values[i]);
        if (!append_list_value(text, item)) return false;
    }
    return WriteString(section, key, text);
}
bool INIClass::WriteTechnoTypes(const char* section, const char* key, const TypeList<TechnoTypeClass*>& values) {
    char text[512]{};
    for (int i = 0; i < values.Count; ++i) {
        if (!values[i] || !append_list_value(text, values[i]->ID)) return false;
    }
    return WriteString(section, key, text);
}
bool INIClass::WriteMovie(const char* section, const char* key, int index) {
    const auto& runtime = game::ini_runtime();
    const char* name = nullptr;
    if (index < 0 || !runtime.name(runtime.context, Kind::movie, index, name) || !name) name = "<none>";
    return WriteString(section, key, name);
}
bool INIClass::WriteTheme(const char* section, const char* key, int index) {
    const auto& runtime = game::ini_runtime();
    const char* name = nullptr;
    if (index < 0 || !runtime.name(runtime.context, Kind::theme, index, name) || !name) name = "No theme";
    return WriteString(section, key, name);
}
