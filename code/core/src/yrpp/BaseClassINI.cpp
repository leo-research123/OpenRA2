// BaseClass::LoadFromINI, YR 0x0042EBE0.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/CCINIClass.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
// strtok over ",": skips leading separators and ends the token at the next.
char* next_token(char*& cursor) {
    while (*cursor == ',')
        ++cursor;
    if (!*cursor) return nullptr;
    char* token = cursor;
    while (*cursor && *cursor != ',')
        ++cursor;
    if (*cursor) *cursor++ = 0;
    return token;
}
// The original passes a missing token to atoi, which faults; it reads as 0.
int number(const char* token) { return token ? std::atoi(token) : 0; }
}

// Each node "NNN=Type,X,Y" names its building type, or gives a negative index
// directly. The original leaves Placed and Attempts as uninitialized stack
// words; they start cleared here. A node the vector cannot take is dropped.
void BaseClass::LoadFromINI(CCINIClass* ini, const char* section) {
    // 0x00526B00: the section lookup is cached by name pointer.
    ini->Reset();
    PercentBuilt = ini->ReadInteger(section, "PercentBuilt", PercentBuilt);
    const int count = ini->ReadInteger(section, "NodeCount", 0);
    for (int i = 0; i < count; ++i) {
        char key[8], value[128];
        std::snprintf(key, sizeof(key), "%03d", i);
        ini->ReadString(section, key, "", value, sizeof(value));
        char* cursor = value;
        const bool numbered = value[0] == '-';
        const char* type = next_token(cursor);
        BaseNodeClass node{};
        node.BuildingTypeIndex = numbered ? number(type) : (type ? BuildingTypeClass::FindIndex(type) : -1);
        const short x = static_cast<short>(number(next_token(cursor)));
        const short y = static_cast<short>(number(next_token(cursor)));
        node.MapCoords = {x, y};
        BaseNodes.AddItem(node);
    }
}
