// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/TerrainTypeClass.h"
#include <cstring>

HRESULT YRPP_STDCALL TerrainTypeClass::GetClassID(CLSID* dest) {
    // Original 0x71e260: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x5af2ce7bu, 0x11d20634u, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType TerrainTypeClass::WhatAmI() const { return AbsID; } // 0x71e330
int TerrainTypeClass::Size() const { return sizeof(*this); } // 0x71e340
static_assert(static_cast<int>(TerrainTypeClass::AbsID) == 37);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TerrainTypeClass) == 0x2bc);
#endif

int TerrainTypeClass::GetArrayIndex() const { return ArrayIndex; }

namespace { DynamicVectorClass<TerrainTypeClass*> types; }
DynamicVectorClass<TerrainTypeClass*>& TerrainTypeClass::Array = types;
TerrainTypeClass* YRPP_FASTCALL TerrainTypeClass::Find(const char* id) {
    const int index = FindIndex(id);
    return index < 0 ? nullptr : Array[index];
}
int YRPP_FASTCALL TerrainTypeClass::FindIndex(const char* id) {
    if (!id) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (_strcmpi(Array[i]->ID, id) == 0) return i;
    return -1;
}

// Supplied 71DA80; values mapped against existing x86 member offsets.
TerrainTypeClass::TerrainTypeClass(const char* id) noexcept : ObjectTypeClass(id), ArrayIndex(-1), Foundation(0), RadarColor(0, 0, 0),
    AnimationRate(0), AnimationProbability(0.0f), TemperateOccupationBits(7), SnowOccupationBits(7),
    WaterBound(false), SpawnsTiberium(false), IsFlammable(false), IsAnimated(false),
    IsVeinhole(false), FoundationData(nullptr) {
    Create_ID();
    Selectable = false; IsLogic = true; RadarInvisible = true;
    LegalTarget = false; Insignificant = true; Strength = -1; Armor = static_cast<::Armor>(6);
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}
// 71E360 contains the destructor body plus scalar-delete epilogue. The C++
// destructor implements only the body; the caller retains deletion ownership.
TerrainTypeClass::~TerrainTypeClass() {
    YRMemory::Deallocate(Image); Image = nullptr;
    NotifyTypeExpired();
    Array.Remove(this);
}

#include "yrpp/CRC.h"
void TerrainTypeClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc);
    crc(ArrayIndex);
    crc(WaterBound);
    crc(SpawnsTiberium);
    crc(IsFlammable);
    crc(Foundation);
    crc(IsAnimated);
    crc(AnimationRate);
    crc(AnimationProbability);
}

#include <new>
// Supplied 0071E2A0; independent native allocation size, same lookup/sentinel order.
TerrainTypeClass* YRPP_FASTCALL TerrainTypeClass::FindOrAllocate(const char* id) {
    if (!id || _strcmpi(id, "none") == 0 || _strcmpi(id, "<none>") == 0) return nullptr;
    if (auto* existing = Find(id)) return existing;
    void* storage = YRMemory::Allocate(sizeof(TerrainTypeClass));
    return storage ? new (storage) TerrainTypeClass(id) : nullptr;
}
