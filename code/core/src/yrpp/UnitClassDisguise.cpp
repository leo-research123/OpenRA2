// SPDX-License-Identifier: GPL-3.0-or-later
// Existing OpenTS baseline license: third_party/opents/LICENSE.md (EA Section 7).
// YR-specific additions to the OpenTS 44fac744 Unit/Techno baseline.
// Mirage disguise is absent from that TS revision. Keep the existing YRpp
// object/mission model; calibrated to gamemd 0x7465B0..0x746B13.
#include "yrpp/UnitClass.h"
#include "techno_spatial.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/Unsorted.h"
#include "map_world.hpp"

DWORD UnitClass::DisguiseAs(AbstractClass* target) {
    if (!target) return 0;
    switch (target->WhatAmI()) {
        case AbstractType::Terrain:
            Disguise = static_cast<ObjectClass*>(target)->GetType();
            DisguisedAsHouse = nullptr;
            break;
        case AbstractType::Cell:
            if (!CanDisguiseAs(target)) return 0;
            Disguise = OverlayTypeClass::Array.GetItemOrDefault(static_cast<CellClass*>(target)->OverlayTypeIndex);
            DisguisedAsHouse = nullptr;
            break;
        case AbstractType::Unit: {
            auto* object = static_cast<ObjectClass*>(target);
            if (!object->IsDisguised()) return 0;
            Disguise = object->GetDisguise(true);
            DisguisedAsHouse = object->GetDisguiseHouse(true);
            break;
        }
        default:
            return 0;
    }
    return TechnoClass::DisguiseAs(target);
}
bool UnitClass::IsDisguisedAs(HouseClass* house) const {
    if (!IsDisguised() || !house || !Owner) return false;
    if (Owner->IsAlliedWith(house) || GetCell()->DisguiseSensors_InclHouse(house->ArrayIndex)) return false;
    return !DisguisedAsHouse || DisguisedAsHouse == house || house->IsAlliedWith(DisguisedAsHouse);
}
ObjectTypeClass* UnitClass::GetDisguise(bool againstAllies) const {
    return againstAllies || !Owner || !Owner->IsAlliedWith(HouseClass::CurrentPlayer) ? Disguise : Type;
}
HouseClass* UnitClass::GetDisguiseHouse(bool againstAllies) const {
    return againstAllies || !Owner || !Owner->IsAlliedWith(HouseClass::CurrentPlayer) ? DisguisedAsHouse : Owner;
}
void UnitClass::ClearDisguise() {
    game::techno_spatial_dirty(this);
    const bool changed = Disguised || Disguise || DisguisedAsHouse;
    Disguised = false;
    Disguise = nullptr;
    DisguisedAsHouse = nullptr;
    if (changed) {
        NeedsRedraw = true;
        game::map_object_changed();
    }
}
void UnitClass::UpdateDisguise() {
    if (!Locomotor || !Owner) return;
    const bool moving = Locomotor->Is_Moving();
    const bool eligible =
        !IsDisguised() && !moving && Type->DisguiseWhenStill && (!RadioLinks.Capacity || !GetNthLink(0));
    if (moving)
        ClearDisguise();
    else {
        bool detected = false;
        // The original branch is frame % 8 != 0, not once every eight frames.
        if (Unsorted::CurrentFrame % 8) {
            const auto here = GetMapCoords();
            const bool alt = GetCell()->Tile_Is_Bridge();
            for (auto delta : Unsorted::AdjacentCell) {
                const auto* cell =
                    MapClass::Instance.TryGetCellAt(CellStruct{short(here.X + delta.X), short(here.Y + delta.Y)});
                auto* infantry = cell ? cell->GetInfantry(alt) : nullptr;
                if (infantry && !Owner->IsAlliedWith(infantry)) {
                    detected = true;
                    break;
                }
            }
        }
        if (detected) {
            InfantryBlinkTimer.Start(RulesClass::Instance->InfantryBlinkDisguiseTime);
            ClearDisguise();
        } else if (eligible && !InfantryBlinkTimer.GetTimeLeft()) {
            auto& choices = RulesClass::Instance->DefaultMirageDisguises;
            if (choices.Count > 0 && ScenarioClass::Instance) {
                const int index = ScenarioClass::Instance->Random.RandomRanged(0, choices.Count - 1);
                if (auto* type = choices[index]) {
                    Disguise = type;
                    DisguisedAsHouse = nullptr;
                    Disguised = true;
                    game::techno_spatial_dirty(this);
                    DisguiseCreationFrame = Unsorted::CurrentFrame;
                    NeedsRedraw = true;
                    game::map_object_changed();
                }
            }
        }
    }
    if (MindControlRingAnim) MindControlRingAnim->Invisible = IsDisguised() && !Owner->IsControlledByCurrentPlayer();
}
