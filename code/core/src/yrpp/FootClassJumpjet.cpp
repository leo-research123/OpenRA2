// Original YR 0x4135A0 / 0x4E00B0. The first returns true for a conflict.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
bool FootClass::Jumpjet_LocationClear() const {
    auto* other = GetCell()->Jumpjet;
    return other && other != this;
}
void FootClass::Jumpjet_OccupyCell(CellStruct cell) {
    if (CurrentJumpjetMapCoords != CellStruct::Empty && cell != CellStruct::Empty) {
        auto* old = MapClass::Instance.GetCellAt(CurrentJumpjetMapCoords);
        if (old->Jumpjet == this) old->Jumpjet = nullptr;
    }
    CurrentJumpjetMapCoords = cell;
}
