#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/VocClass.h"
#include <bit>
#include <cstdint>

bool UnitClass::vt_entry_29C() {
    return !UnloadTimer.AreStates11() && !UnloadTimer.AreStates10() && !UnloadTimer.AreStates01();
}

void UnitClass::TryCrushCell(const CellStruct& where, bool warn) {
    auto* cell = MapClass::Instance.GetCellAt(where);
    bool bridge = false;
    if ((static_cast<unsigned>(cell->Flags) & 0x100u) != 0) {
        bridge = OnBridge || static_cast<signed char>(MapClass::Instance.GetCellAt(GetCoords())->Level) ==
                                 static_cast<signed char>(cell->Level) + 4;
    }
    if (!Type->Crusher && !HasAbility(Ability::Crusher)) return;
    if (warn) {
        if ((bridge ? cell->AltOccupationFlags : cell->OccupationFlags) & 0x1Fu)
            cell->ScatterContent(CoordStruct::Empty, true, false, bridge);
        return;
    }
    bool crushed = false;
    bool crushedVehicle = false;
    auto* object = bridge ? cell->AltObject : cell->FirstObject;
    while (object) {
        if (!object->IsCrushable(this) || (Owner->IsAlliedWith(object) && !Type->IsTrain)) {
            object = object->NextObject;
            continue;
        }
        const auto victim = object->GetCoords();
        const auto own = GetCoords();
        // 0x5F6560 uses wrapping 32-bit arithmetic and a signed comparison.
        const auto dx = std::uint32_t(own.X) - std::uint32_t(victim.X);
        const auto dy = std::uint32_t(own.Y) - std::uint32_t(victim.Y);
        if (std::bit_cast<std::int32_t>(dx * dx + dy * dy) >= 0x4000 || object->IsFallingDown) {
            object = object->NextObject;
            continue;
        }
        const bool thief = object->WhatAmI() == AbstractType::Infantry &&
                           static_cast<InfantryClass*>(object)->Type->VehicleThief &&
                           static_cast<InfantryClass*>(object)->Destination == this && !Type->IsTrain;
        auto* next = object->NextObject;
        if (thief) {
            auto* infantry = static_cast<InfantryClass*>(object);
            IsOwnedByCurrentPlayer = infantry->IsOwnedByCurrentPlayer;
            Disappear(false);
            SetOwningHouse(infantry->Owner, true);
            infantry->UnInit();
        } else {
            crushed = true;
            if (object->WhatAmI() == AbstractType::Unit) crushedVehicle = true;
            const auto at = Location;
            VocClass::PlayAt(object->GetType()->CrushSound, at, nullptr);
            object->Destroy();
            object->RegisterDestruction(this);
            object->Mark(MarkType::Up);
            object->Limbo();
            object->UnInit();
        }
        object = next;
    }
    if (crushed) {
        Uncloak(false);
        if (crushedVehicle && RockingForwardsPerFrame == 0.0f) RockingForwardsPerFrame = -0.05f;
    }
}
