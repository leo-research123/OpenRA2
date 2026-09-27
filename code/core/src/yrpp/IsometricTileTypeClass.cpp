// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/IsometricTileTypeClass.h"

namespace { DynamicVectorClass<IsometricTileTypeClass::TileInsertType*> tile_insertions; }
DynamicVectorClass<IsometricTileTypeClass::TileInsertType*>& IsometricTileTypeClass::TileInsertions = tile_insertions;
int YRPP_FASTCALL IsometricTileTypeClass::ConvertTileIndex(int originalIndex) {
    if (originalIndex == 0xffff) return originalIndex;
    auto result = static_cast<std::uint32_t>(originalIndex);
    for (int i = 0; i < TileInsertions.Count; ++i) {
        const auto& insertion = *TileInsertions[i];
        if (insertion.OriginalIndex > originalIndex) break;
        result += static_cast<std::uint32_t>(insertion.Offset);
    }
    return static_cast<std::int32_t>(result);
}
#include <cstring>

HRESULT YRPP_STDCALL IsometricTileTypeClass::GetClassID(CLSID* dest) {
    // Original 0x549d90: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x5af2ce7au, 0x11d20634u, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType IsometricTileTypeClass::WhatAmI() const { return AbsID; } // 0x54a140
int IsometricTileTypeClass::Size() const { return sizeof(*this); } // 0x54a150
static_assert(static_cast<int>(IsometricTileTypeClass::AbsID) == 18);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(IsometricTileTypeClass) == 0x30c);
#endif

int IsometricTileTypeClass::GetArrayIndex() const { return ArrayIndex; }

namespace { DynamicVectorClass<IsometricTileTypeClass*> types; }
DynamicVectorClass<IsometricTileTypeClass*>& IsometricTileTypeClass::Array = types;
DynamicVectorClass<AbstractClass*>& IsometricTileTypeClass::AllTypes = AbstractClass::TypeExpirationListeners;
void (*IsometricTileTypeClass::PointerExpirationObserver)(AbstractClass*) noexcept = nullptr;

IsometricTileTypeClass::IsometricTileTypeClass(int index, int theater, int tile,
        const char* name, int variant) noexcept : ObjectTypeClass(name),
    ArrayIndex(index), MarbleMadnessTile(0xffff), NonMarbleMadnessTile(0xffff),
    unk_2A0(0), NextVariant(nullptr), ToSnowTheater(-1), ToTemperateTheater(-1),
    TileAnimIndex(-1), TileXOffset(0), TileYOffset(0), TileAttachesTo(-1), TileZAdjust(0),
    unk_2DC(static_cast<byte>(theater)), Morphable(false), ShadowCaster(false),
    AllowToPlace(true), RequiredByRMG(false), unk_2E4(0), unk_2E8(0),
    unk_2EC(static_cast<byte>(tile)), unk_2F0(1), unk_2F4(false), FileName{},
    AllowBurrowing(true), AllowTiberium(false), unk_308(0) {
    AllTypes.AddItem(this);
    if (!static_cast<byte>(variant)) Array.AddItem(this);
    std::strncpy(Name, name ? name : "", sizeof(Name) - 1);
    Name[sizeof(Name) - 1] = 0;
    RadarInvisible = Insignificant = Immune = true;
    Selectable = LegalTarget = AllowCellContent = false;
}
IsometricTileTypeClass::~IsometricTileTypeClass() {
    if (PointerExpirationObserver) PointerExpirationObserver(this);
    auto* next = NextVariant; NextVariant = nullptr;
    GameDelete(next);
    Array.Remove(this); AllTypes.Remove(this);
    // 544B4A destroys radar entries in reverse order; the ObjectType base
    // removes its own registration before freeing Image (5F7400).
    for (int i = unk_2A4.Count - 1; i >= 0; --i) YRMemory::Deallocate(unk_2A4[i]);
    unk_2A4.Clear();
}

#include "yrpp/AnimTypeClass.h"
void IsometricTileTypeClass::PointerExpired(AbstractClass* object, bool) { // 549DD0
    if (NextVariant == object) NextVariant = nullptr;
    if (TileAnimIndex >= 0 && TileAnimIndex < AnimTypeClass::Array.Count &&
        AnimTypeClass::Array[TileAnimIndex] == object) TileAnimIndex = -1;
}
