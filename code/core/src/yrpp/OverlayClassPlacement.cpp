// Original OverlayClass constructor/Unlimbo/Mark, 0x5FC380/0x5FD270/0x5FC570.
// Live wall placement; packed-map loading remains in OverlayClassINI.cpp.
#include "background_invalidation.hpp"
#include "yrpp/OverlayClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/RulesClass.h"
#include "map_world.hpp"
#include <cstdlib>
#include <vector>

namespace {
// Original 0x8333BC: only constructor placement installs this transient flag.
int placement_flag = -1;
}

OverlayClass::OverlayClass(OverlayTypeClass* type, const CellStruct& cell, int flag) noexcept
    : ObjectClass(), Type(type) {
    Create_ID();
    if (!Array.AddItem(this)) std::abort();
    if (cell != CellStruct{-1, -1}) {
        placement_flag = flag;
        const CoordStruct where{cell.X * 256 + 128, cell.Y * 256 + 128, 0};
        if (!MapClass::Instance.GetCellAt(cell)->GetTerrain(false)) ObjectClass::Unlimbo(where, DirType::North);
        placement_flag = -1;
    }
}

OverlayClass::~OverlayClass() {
    NotifyObjectExpired(true);
    Array.Remove(this);
    if (Game::IsActive) ObjectClass::Limbo();
    Type = nullptr;
}

bool OverlayClass::Unlimbo(const CoordStruct& where, DirType facing) {
    if (MapClass::Instance.GetCellAt(where)->GetTerrain(false)) return false;
    return ObjectClass::Unlimbo(where, facing);
}

bool OverlayClass::Mark(MarkType mark) {
    if (!ObjectClass::Mark(mark) || (mark != MarkType::Down && mark != MarkType::ChangeRedraw)) return false;
    auto* cell = GetCell();
    if (cell->SlopeIndex > 4 && Type->ArrayIndex != OVERLAY_VEINHOLEDUMMY) return false;
    // YR 0x005FC570: high bridge overlays restore the original deck cells.
    const int overlay = Type->ArrayIndex;
    if (overlay == 24 || overlay == 25 || overlay == 237 || overlay == 238) {
        const auto direction = (overlay == 24 || overlay == 237) ? FacingType::North : FacingType::West;
        cell->InitializeBridge(direction);
        auto* next = cell;
        for (int i = 0; i < 3; ++i) {
            RadarClass::Instance.RadarBackground(next->MapCoords);
            next = next->GetNeighbourCell(direction);
        }
        RadarClass::Instance.RadarBackground(
            cell->GetNeighbourCell(static_cast<FacingType>((static_cast<unsigned>(direction) + 4u) & 7u))->MapCoords);
        cell->OverlayTypeIndex = overlay;
        cell->RecalcAttributes(-1);
        game::map_resource_changed(*cell);
        IsOnMap = false;
        InLimbo = true;
        UnInit();
        return true;
    }
    // Crates use the original non-wall placement arm and become cell data.
    if (Type->Crate) {
        const auto speed =
            Type == RulesClass::Instance->WaterCrateImg ? static_cast<SpeedType>(5) : static_cast<SpeedType>(1);
        if (Unsorted::ScenarioInit ||
            cell->IsClearToMove(speed, false, false, -1, static_cast<MovementZone>(0), -1, true)) {
            cell->OverlayTypeIndex = Type->ArrayIndex;
            cell->OverlayData = 0xFF;
        }
        cell->RecalcAttributes(-1);
        game::map_resource_changed(*cell);
        IsOnMap = false;
        InLimbo = true;
        UnInit();
        return true;
    }
    // 0x5FC6E6..0x5FC775. Walls become cell data, never cell-chain objects.
    if (!Type->Wall) return false;
    if (!cell->CanThisExistHere(SpeedType::Track, nullptr, nullptr)) {
        UnInit();
        return false;
    }
    cell->OverlayData = 0;
    cell->OverlayTypeIndex = Type->ArrayIndex;
    cell->UpdateWall(true);
    if (!Unsorted::ScenarioInit) {
        MapClass::Instance.RecalculateZones(cell->MapCoords);
        MapClass::Instance.RecalculateSubZones(cell->MapCoords);
    }
    if (placement_flag != -1) cell->WallOwnerIndex = DisplayClass::Instance.CurrentBuildingOwnerArrayIndex;
    for (int direction = 0; direction < 8; ++direction)
        ++cell->GetNeighbourCell(static_cast<FacingType>(direction))->BlockedNeighbours;
    cell->RecalcAttributes(-1);
    game::map_resource_changed(*cell);
    // 0x5FD20B: prevent Limbo from removing the newly installed cell overlay.
    IsOnMap = false;
    InLimbo = true;
    UnInit();
    return true;
}

#if !defined(RA2_YRPP_GAME)
// YR 0x5FDDF0.
void YRPP_FASTCALL OverlayClass::RebuildVeins() {
    constexpr int Veins = 126;
    auto& map = MapClass::Instance;
    std::vector<CellClass*> full;
    map.CellIteratorReset();
    while (auto* cell = map.CellIteratorNext()) {
        if (cell->OverlayTypeIndex != Veins) continue;
        if (cell->OverlayData >= 0x30) full.push_back(cell);
        game::invalidate_background_cell(cell->MapCoords, 0x6);
        cell->OverlayTypeIndex = -1;
        cell->OverlayData = 0;
    }
    for (auto i = full.size(); i-- > 0;)
        if (full[i]->CanHaveVeins()) full[i]->PlaceVeins();
}
#endif
