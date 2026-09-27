// Compile-only Windows x86 Microsoft ABI contract. No new object fields/slots.
#include "yrpp/BuildingClass.h"
#include "yrpp/DisplayClass.h"
#include <cstddef>

static_assert(sizeof(void*) == 4);
static_assert(sizeof(BuildingTypeClass) == 0x1798);
static_assert(sizeof(CellClass) == 0x148);
static_assert(sizeof(DisplayClass) == 0x11E8);
static_assert(offsetof(BuildingTypeClass, BuildCat) == 0xE08);
static_assert(offsetof(BuildingTypeClass, Adjacent) == 0xEB4);
static_assert(offsetof(BuildingTypeClass, FoundationData) == 0xDFC);
static_assert(offsetof(BuildingTypeClass, BaseNormal) == 0x154F);
static_assert(offsetof(BuildingTypeClass, EligibileForAllyBuilding) == 0x1550);
static_assert(offsetof(BuildingTypeClass, ConstructionYard) == 0x16B9);
static_assert(offsetof(BuildingTypeClass, PlaceAnywhere) == 0x1703);
static_assert(offsetof(BuildingClass, Type) == 0x520);
static_assert(offsetof(BuildingClass, IsOnMap) == 0x74);
static_assert(offsetof(CellClass, OccupationFlags) == 0x124);
static_assert(offsetof(CellClass, Flags) == 0x140);

// LLVM IR must load virtual slot 42 (0x00A8) and slot 107 (0x01AC).
extern "C" __declspec(dllexport) bool type_placement_slot(
    const TechnoTypeClass* type, const CellStruct& cell, HouseClass* owner) {
    return type->CanCreateHere(cell, owner);
}
extern "C" __declspec(dllexport) Move building_placement_slot(
    const ObjectClass* object, CellClass* cell) {
    return object->IsCellOccupied(cell, FacingType::None, -1, nullptr, false);
}
