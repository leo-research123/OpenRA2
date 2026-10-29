// Original YR 0x487D70 / 0x485080 / 0x481130.
#include "yrpp/CellClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/Unsorted.h"
bool CellClass::TryAssignJumpjet(FootClass* object) {
    if (object) {
        if (Jumpjet && Jumpjet != object) return false;
        Jumpjet = object;
        if ((object->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None) object->Jumpjet_OccupyCell(MapCoords);
    } else {
        if (Jumpjet && (Jumpjet->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None)
            Jumpjet->Jumpjet_OccupyCell(CellStruct::Empty);
        Jumpjet = nullptr;
    }
    return true;
}
int CellClass::GetJumpjetHeight() const {
    const int floor = GetFloorHeight({128, 128});
    if (auto* building = GetBuilding()) {
        CoordStruct dimensions;
        return floor + building->Type->Dimension2(&dimensions)->Z;
    }
    return floor + (FindTechnoNearestTo({0, 0}, false, nullptr) ? 85 : 0);
}
bool CellClass::IsInfantrySubpositionFree(unsigned index, bool alt) const {
    return index >= 2 && (BYTE((1u << (index & 31)) & (alt ? AltOccupationFlags : OccupationFlags)) == 0);
}
