// SPDX-License-Identifier: GPL-3.0-or-later
// Existing OpenTS baseline license: third_party/opents/LICENSE.md (EA Section 7).
// YR-specific additions to the OpenTS 44fac744 Unit/Techno baseline.
// Mirage disguise is absent from that TS revision. Keep the existing YRpp
// object/mission model; calibrated to gamemd 0x7465B0..0x746B13.
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/Unsorted.h"
#include "map_world.hpp"

bool UnitClass::IsDisguisedAs(HouseClass* house) const {
    if(!IsDisguised() || !house || !Owner)return false;
    if(Owner->IsAlliedWith(house) || GetCell()->DisguiseSensors_InclHouse(house->ArrayIndex))return false;
    return !DisguisedAsHouse || DisguisedAsHouse==house || house->IsAlliedWith(DisguisedAsHouse);
}
ObjectTypeClass* UnitClass::GetDisguise(bool againstAllies) const {
    return againstAllies || !Owner || !Owner->IsAlliedWith(HouseClass::CurrentPlayer)?Disguise:Type;
}
HouseClass* UnitClass::GetDisguiseHouse(bool againstAllies) const {
    return againstAllies || !Owner || !Owner->IsAlliedWith(HouseClass::CurrentPlayer)?DisguisedAsHouse:Owner;
}
void UnitClass::ClearDisguise() {
    const bool changed=Disguised || Disguise || DisguisedAsHouse;
    Disguised=false;Disguise=nullptr;DisguisedAsHouse=nullptr;
    if(changed){NeedsRedraw=true;game::map_object_changed();}
}
void UnitClass::UpdateDisguise() {
    if(!Locomotor || !Owner)return;
    const bool moving=Locomotor->Is_Moving();
    const bool eligible=!IsDisguised() && !moving && Type->DisguiseWhenStill
        && (!RadioLinks.Capacity || !GetNthLink(0));
    if(moving)ClearDisguise();
    else {
        bool detected=false;
        // The original branch is frame % 8 != 0, not once every eight frames.
        if(Unsorted::CurrentFrame%8) {
            const auto here=GetMapCoords();const bool alt=GetCell()->Tile_Is_Bridge();
            for(auto delta:Unsorted::AdjacentCell) {
                const auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{short(here.X+delta.X),short(here.Y+delta.Y)});
                auto* infantry=cell?cell->GetInfantry(alt):nullptr;
                if(infantry && !Owner->IsAlliedWith(infantry)){detected=true;break;}
            }
        }
        if(detected) {
            InfantryBlinkTimer.Start(RulesClass::Instance->InfantryBlinkDisguiseTime);ClearDisguise();
        }else if(eligible && !InfantryBlinkTimer.GetTimeLeft()) {
            auto& choices=RulesClass::Instance->DefaultMirageDisguises;
            if(choices.Count>0 && ScenarioClass::Instance) {
                const int index=ScenarioClass::Instance->Random.RandomRanged(0,choices.Count-1);
                if(auto* type=choices[index]) {
                    Disguise=type;DisguisedAsHouse=nullptr;Disguised=true;DisguiseCreationFrame=Unsorted::CurrentFrame;
                    NeedsRedraw=true;game::map_object_changed();
                }
            }
        }
    }
    if(MindControlRingAnim)MindControlRingAnim->Invisible=IsDisguised()&&!Owner->IsControlledByCurrentPlayer();
}
