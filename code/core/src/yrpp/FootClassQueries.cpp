// YR Foot virtual overrides; preserve the original object and vtable layout.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#if !defined(RA2_YRPP_GAME)
bool FootClass::HaveAttackMoveTarget() const { return HasAttackMoveTarget; }
Mission FootClass::GetMegaMission() const { return MegaMission; }
CoordStruct* FootClass::GetAttackMoveCoords(CoordStruct* output) {
    if (MegaDestination)
        *output = MegaDestination->GetCoords();
    else if (MegaTarget)
        *output = MegaTarget->GetCoords();
    else
        *output = {-1, -1, -1};
    return output;
}
CoordStruct* FootClass::GetAttackCoordinates(CoordStruct* output) const {
    if (GetCurrentMission() != Mission::Attack && (unknown_5A0 || Destination))
        *output = (unknown_5A0 ? unknown_5A0 : Destination)->GetCenterCoords();
    else
        TechnoClass::GetAttackCoordinates(output);
    return output;
}
bool FootClass::vt_entry_4E0() const {
    if (!GetWeapon(0)) return true;
    if (GetTechnoType()->TechLevel == -1) return true;
    if (GetTechnoType()->DeploysInto) return true;
    return GetTechnoType()->Speed <= 13;
}
SHPStruct* FootClass::GetImage() const {
    const auto at = GetCoords();
    auto* cell = MapClass::Instance.GetCellAt(CellStruct{short(at.X / 256), short(at.Y / 256)});
    auto* player = HouseClass::CurrentPlayer;
    if (!Owner->IsAlliedWith(player) && IsDisguised() && !cell->DisguiseSensors_InclHouse(player->ArrayIndex))
        return GetDisguise(false)->Image;
    if (IsClearlyVisibleTo(player)) return ObjectClass::GetImage();
    return GetDisguise(true)->Image;
}
#endif
