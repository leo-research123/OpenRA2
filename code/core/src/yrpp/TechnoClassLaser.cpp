// YR 0x006FD210: the same laser creation used by main shots and shrapnel.
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/WeaponTypeClass.h"
#include <algorithm>
#include <bit>

#if !defined(RA2_YRPP_GAME)

LaserDrawClass* TechnoClass::CreateLaser(ObjectClass* target, int index, WeaponTypeClass* weapon,
                                         const CoordStruct& coords) {
    const int duration = static_cast<signed char>(weapon->LaserDuration);
    const bool houseColor = weapon->IsHouseColor;
    const auto inner = houseColor ? Owner->LaserColor : weapon->LaserInnerColor;
    const auto outer =
        houseColor ? ColorStruct{BYTE(inner.R >> 1), BYTE(inner.G >> 1), BYTE(inner.B >> 1)} : weapon->LaserOuterColor;
    auto source = coords;
    int depth = 0;
    // 0x6F2A50 initializes the original implicit-source marker to zero.
    if (coords == CoordStruct::Empty) {
        GetFLH(&source, index, CoordStruct::Empty);
        if (WhatAmI() == AbstractType::Building) {
            const auto firePixel = TacticalClass::CoordsToScreen(source);
            const auto basePixel = TacticalClass::CoordsToScreen(GetRenderCoords());
            depth = std::min(std::bit_cast<int>(unsigned(firePixel.Y) - unsigned(basePixel.Y)), 0);
        }
    }
    // The existing pointer ABI also accepts CellClass targets through AbstractClass.
    auto* abstract = reinterpret_cast<AbstractClass*>(target);
    CoordStruct end;
    if ((abstract->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None)
        target->GetTargetCoords(&end);
    else
        abstract->GetCenterCoords(&end);
    auto* memory = YRMemory::Allocate(sizeof(LaserDrawClass));
    if (!memory) return nullptr;
    auto* laser =
        ::new (memory) LaserDrawClass(source, end, depth, 1, inner, outer, weapon->LaserOuterSpread, duration);
    if (houseColor) laser->IsHouseColor = true;
    return laser;
}

#endif
