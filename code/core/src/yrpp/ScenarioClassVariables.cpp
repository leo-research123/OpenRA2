// EA SCENARIO.CPP f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae variable model;
// YR 6896C0..689CE0 / 68BDC0..68BF19 add names, local values and cell waypoints.
#include "yrpp/YRPPCore.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CellClass.h"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
template<unsigned N> int find_variable(Variable (&variables)[N], const char* name) {
    for (unsigned i = 0; i < N; ++i) if (!std::strcmp(name, variables[i].Name)) return static_cast<int>(i);
    return -1;
}
// strtok-compatible comma token traversal without changing process-wide state.
char* token(char*& cursor) {
    if (!cursor) return nullptr;
    while (*cursor == ',') ++cursor;
    if (!*cursor) { cursor = nullptr; return nullptr; }
    char* result = cursor;
    while (*cursor && *cursor != ',') ++cursor;
    if (*cursor) *cursor++ = '\0'; else cursor = nullptr;
    return result;
}
}
int ScenarioClass::FindGlobal(const char* name) { return find_variable(GlobalVariables, name); }
int ScenarioClass::FindLocal(const char* name) { return find_variable(LocalVariables, name); }
char ScenarioClass::SetGlobal(const char* name, char value) {
    const int index = FindGlobal(name); return index == -1 ? 1 : SetGlobal(index, value);
}
char ScenarioClass::SetLocal(const char* name, char value) {
    const int index = FindLocal(name); return index == -1 ? 1 : SetLocal(index, value);
}
bool ScenarioClass::GetGlobal(const char* name, char* value) { return GetGlobal(FindGlobal(name), value); }
bool ScenarioClass::GetLocal(const char* name, char* value) { return GetLocal(FindLocal(name), value); }
bool ScenarioClass::ReadGlobalVariables(INIClass& ini) {
    const int count = std::min(50, ini.GetKeyCount("VariableNames"));
    for (int i = 0; i < count; ++i) {
        const char* key = ini.GetKeyName("VariableNames", i);
        const int index = std::atoi(key);
        if (static_cast<unsigned>(index) >= 50u) return false;
        ini.ReadString("VariableNames", key, nullptr, GlobalVariables[index].Name, 40);
    }
    return true;
}
bool ScenarioClass::ReadLocalVariables(INIClass& ini) {
    for (auto& variable : LocalVariables) variable.Name[0] = '\0';
    const int count = std::min(100, ini.GetKeyCount("VariableNames"));
    for (int i = 0; i < count; ++i) {
        const char* key = ini.GetKeyName("VariableNames", i);
        const int index = std::atoi(key);
        if (static_cast<unsigned>(index) >= 100u) return false;
        char text[128];
        ini.ReadString("VariableNames", key, nullptr, text, sizeof(text));
        char* cursor = text;
        const char* name = token(cursor);
        if (!name || std::strlen(name) >= sizeof(LocalVariables[index].Name)) return false;
        std::strcpy(LocalVariables[index].Name, name);
        if (const char* value = token(cursor)) LocalVariables[index].Value = std::atoi(value) != 0;
    }
    return true;
}
bool ScenarioClass::WriteLocalVariables(INIClass& ini) {
    ini.Clear("VariableNames");
    bool success = true;
    for (int i = 0; i < 100; ++i) {
        const auto& variable = LocalVariables[i];
        if (!variable.Name[0]) continue;
        char key[12], value[128];
        std::snprintf(key, sizeof(key), "%d", i);
        std::snprintf(value, sizeof(value), "%s,%d", variable.Name, variable.Value != 0);
        success = ini.WriteString("VariableNames", key, value) && success;
    }
    return success;
}
bool ScenarioClass::ReadWaypoints(INIClass& ini) {
    const auto& runtime = game::scenario_runtime();
    if (!runtime.cell_at) return false;
    for (int i = 0; i < 702; ++i) {
        char key[20]; std::snprintf(key, sizeof(key), "%d", i);
        const int encoded = ini.ReadInteger("Waypoints", key, 0);
        Waypoints[i].X = static_cast<short>(encoded % 1000);
        Waypoints[i].Y = std::bit_cast<short>(static_cast<unsigned short>(encoded / 1000));
        if (IsDefinedWaypoint(i)) {
            CellClass* cell = nullptr;
            if (!runtime.cell_at(runtime.context, Waypoints[i], cell) || !cell) return false;
            cell->Flags |= CellFlags::IsWaypoint;
        }
    }
    return true;
}
bool ScenarioClass::WriteWaypoints(INIClass& ini) {
    ini.Clear("Waypoints");
    bool success = true;
    for (int i = 0; i < 702; ++i) if (IsDefinedWaypoint(i)) {
        char key[32]; std::snprintf(key, sizeof(key), "%d", i);
        success = ini.WriteInteger("Waypoints", key, Waypoints[i].X + 1000 * Waypoints[i].Y) && success;
    }
    return success;
}
