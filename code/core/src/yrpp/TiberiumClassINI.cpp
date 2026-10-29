// Original resource INI, list reader and identity: 0x00721A50 / 0x00721D10 / 0x005FDD20.
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/Memory.h"
#include <cstdlib>
#include <algorithm>
bool TiberiumClass::LoadFromINI(CCINIClass* ini) {
    if (!ini || !AbstractTypeClass::LoadFromINI(ini)) return false;
    Spread = ini->ReadInteger(ID, "Spread", Spread);
    Growth = ini->ReadInteger(ID, "Growth", Growth);
    SpreadPercentage = ini->ReadDouble(ID, "SpreadPercentage", SpreadPercentage);
    GrowthPercentage = ini->ReadDouble(ID, "GrowthPercentage", GrowthPercentage);
    Value = std::max(0, ini->ReadInteger(ID, "Value", Value));
    Power = ini->ReadInteger(ID, "Power", Power);
    int image = ini->ReadInteger(ID, "Image", 1);
    int base = image == 2 ? 27 : image == 3 ? 127 : image == 4 ? 147 : 102;
    Image = OverlayTypeClass::Array.GetItemOrDefault(base);
    NumFrames = 12;
    NumImages = 12;
    NumSlopes = image == 2 ? 0 : 8;
    return true;
}
#if !defined(RA2_YRPP_GAME)
bool YRPP_FASTCALL TiberiumClass::LoadFromINIList(CCINIClass* ini) {
    const int count = ini->GetKeyCount("Tiberiums");
    for (int i = 0; i < count; ++i) {
        const char* key = ini->GetKeyName("Tiberiums", i);
        char name[24];
        if (ini->ReadString("Tiberiums", key, "", name, sizeof(name)) <= 0) continue;
        // The key number selects the slot; the listed name only names a new slot.
        const int slot = std::atoi(key);
        if (slot < 0) continue;  // The original indexes before the registry; skip that slot.
        auto* type = slot >= Array.Count ? GameCreate<TiberiumClass>(name) : Array[slot];
        type->LoadFromINI(ini);
    }
    return true;
}
#endif
