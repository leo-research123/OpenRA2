// YR 0x004D2370 FoggedObjectClass::UpdateAll: every remembered object marks the
// cells it covers as fogged. Maps without FogOfWar never register one, so on
// this host the loop runs over an empty array.
#include "yrpp/FoggedObjectClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/MapClass.h"

#if !defined(RA2_YRPP_GAME)
void YRPP_CDECL FoggedObjectClass::UpdateAll() {
    const auto mark = [](CellClass* cell) {
        if (cell) cell->Flags |= CellFlags::Fogged;
    };
    // The count is re-read each pass, as at 0x4D2381.
    for (int i = 0; i < Array.Count; ++i) {
        auto* fogged = Array[i];
        const CellStruct* foundation = nullptr;
        if (fogged->RTTI == AbstractType::Building && fogged->Records.Count > 0 && fogged->Records.Items[0].TypeClass)
            foundation = fogged->Records.Items[0].TypeClass->GetFoundationData(false);
        if (!foundation) {
            mark(MapClass::Instance.GetCellAt(fogged->Position));
            continue;
        }
        CellStruct buffer{};
        const CellStruct* origin = fogged->GetCell(&buffer);
        if (!origin) continue;
        const CellStruct base = *origin;
        for (; foundation->X != 0x7FFF || foundation->Y != 0x7FFF; ++foundation)
            mark(MapClass::Instance.GetCellAt(
                CellStruct{static_cast<short>(base.X + foundation->X), static_cast<short>(base.Y + foundation->Y)}));
    }
}
#endif
