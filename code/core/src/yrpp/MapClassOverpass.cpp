// YR 0x00568BB0 Overpass and 0x004818E0 Cell Tiberium_Adjust.
#include "background_invalidation.hpp"
#include "yrpp/MapClass.h"
#include <climits>
#include "yrpp/HouseTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TiberiumClass.h"

#if !defined(RA2_YRPP_GAME)
namespace {
constexpr unsigned HorizontalLineFlag = 0x100000u, VerticalLineFlag = 0x200000u, VeinAttackFlag = 0x20000u;
void set_flag(CellClass& cell, unsigned flag) {
    cell.Flags = static_cast<CellFlags>(static_cast<unsigned>(cell.Flags) | flag);
}
void clear_flags(CellClass& cell, unsigned flags) {
    cell.Flags = static_cast<CellFlags>(static_cast<unsigned>(cell.Flags) & ~flags);
}
}

int CellClass::TiberiumAdjust(bool pregame) {
    if (OverlayTypeIndex == -1) return 0;
    const int index = TiberiumClass::FindIndex(OverlayTypeIndex);
    if (index == -1) return 0;
    auto* type = TiberiumClass::Array[index];
    const int value = type->Value;
    if (pregame) {
        // The original applies this index pick to the Image pointer itself
        // (Image .. Image + NumImages - 1 as OverlayTypeClass addresses) and
        // stores the address. The same single draw over the image indices
        // keeps the random stream and yields the intended overlay.
        const int first = type->Image->ArrayIndex;
        OverlayTypeIndex = ScenarioClass::Instance->Random.RandomRanged(first, first + type->NumImages - 1);
    }
    int neighbours = 0;
    for (int direction = 0; direction < 8; ++direction)
        if (TiberiumClass::FindIndex(GetNeighbourCell(static_cast<FacingType>(direction))->OverlayTypeIndex) == index)
            ++neighbours;
    static constexpr unsigned char density[]{0, 1, 3, 4, 6, 7, 8, 10, 11};
    OverlayData = density[neighbours % type->NumFrames];
    game::invalidate_background_cell(MapCoords, 0x6);
    return value * (OverlayData + 1);
}

void CellClass::SetWallOwner() {
    if (OverlayTypeIndex == -1 || !OverlayTypeClass::Array[OverlayTypeIndex]->Wall) return;
    int owner = -1, nearest = INT_MAX;
    for (int i = 0; i < BuildingClass::Array.Count; ++i) {
        auto* building = BuildingClass::Array[i];
        if (!building->IsAlive || !building->IsOnMap || !building->Owner->Type->WallOwner) continue;
        const int distance = building->DistanceFrom3D(this);
        if (distance < nearest) {
            nearest = distance;
            owner = building->GetOwningHouseIndex();
        }
    }
    WallOwnerIndex = owner;
    game::invalidate_background_cell(MapCoords, 0x6);
}
int MapClass::Overpass(bool pregame) {
    game::invalidate_background_all();
    for (int i = 0; i < AnimClass::Array.Count; ++i) {
        auto* anim = AnimClass::Array[i];
        if (anim->DeleteOnMapCleanup) {
            GameDelete(anim);
            --i;
        }
    }
    CellIteratorReset();
    while (auto* cell = CellIteratorNext())
        clear_flags(*cell, HorizontalLineFlag | VerticalLineFlag);
    int total = 0;
    CellIteratorReset();
    while (auto* cell = CellIteratorNext()) {
        cell->unknown_30 = nullptr;
        cell->InitLightConvert(nullptr, 0x10000, 0, 1000, 1000, 1000);
        clear_flags(*cell, VeinAttackFlag);
        if (auto* tag = cell->AttachedTag) {
            if (tag->HasCrossesHorizontalLineEvent()) {
                for (int x = 0; x < MapCoordBounds.Right; ++x)
                    set_flag(*GetCellAt(CellStruct{short(x + MapCoordBounds.Left), cell->MapCoords.Y}),
                             HorizontalLineFlag);
            } else if (tag->HasCrossesVerticalLineEvent()) {
                for (int y = 0; y < MapCoordBounds.Bottom; ++y)
                    set_flag(*GetCellAt(CellStruct{cell->MapCoords.X, short(y + MapCoordBounds.Top)}),
                             VerticalLineFlag);
            }
        }
        total += pregame ? cell->TiberiumAdjust(false) : cell->GetContainedTiberiumValue();
        cell->RecalcAttributes(-1);
        if (cell->OverlayTypeIndex != -1 && OverlayTypeClass::Array[cell->OverlayTypeIndex]->Wall) cell->SetWallOwner();
    }
    return total;
}
#endif
