// Original BuildingClass::CanBeSelectedNow (group selection), YR 0x459C00.
// 0x465D40 requires an undeploy target and a 1x1 foundation, not TS's yard flag.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
bool BuildingClass::CanBeOccupiedBy(InfantryClass* infantry) const {
    if(!infantry||!Type->CanBeOccupied||CurrentMission==Mission::Construction||CurrentMission==Mission::Selling
        ||!MapClass::Instance.IsWithinUsableArea(GetCoords())||IsBeingWarpedOut())return false;
    if(!infantry->Type->Occupier)return infantry->Type->Assaulter&&!Owner->IsAlliedWith(infantry)&&GetOccupantCount();
    return (Owner==infantry->Owner||Owner->Type->MultiplayPassive)&&GetOccupantCount()!=Type->MaxNumberOccupants
        &&!IsRedHP()&&!infantry->MindControlledBy&&!infantry->MindControlledByAUnit;
}
bool BuildingClass::CanBeSelectedNow() const {
    return Type->UndeploysInto && Type->GetFoundationWidth()==1 && Type->GetFoundationHeight(false)==1
        && TechnoClass::CanBeSelectedNow();
}
