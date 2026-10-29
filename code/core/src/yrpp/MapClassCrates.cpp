// Original crate placement and removal, YR 0x0056BEC0 / 0x004A17C0.
#include "yrpp/MapClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "x87_integer.hpp"
#include <cstdlib>

bool Crate::Remove() {
    if (Location == CellStruct{0, 0}) return false;
    auto* cell = MapClass::Instance.TryGetCellAt(Location);
    if (cell && MapClass::Instance.CoordinatesLegal(Location) && cell->OverlayTypeIndex != -1) {
        auto* type = OverlayTypeClass::Array.GetItemOrDefault(cell->OverlayTypeIndex);
        auto& rules = *RulesClass::Instance;
        if (type && (type == rules.CrateImg || type == rules.WoodCrateImg || type == rules.WaterCrateImg)) {
            cell->OverlayTypeIndex = -1;
            cell->OverlayData = 0;
            cell->MarkForRedraw();
        }
    }
    Location = {0, 0};
    CrateTimer.Pause();
    return true;
}
bool Crate::Put(const CellStruct& where) {
    Remove();
    const int initialized = Unsorted::ScenarioInit;
    Unsorted::ScenarioInit = 0;
    auto& map = MapClass::Instance;
    auto& rules = *RulesClass::Instance;
    if (!map.IsWithinUsableArea(where, true) || map.GetCellAt(where)->OverlayTypeIndex != -1) {
        Unsorted::ScenarioInit = initialized;
        return false;
    }
    auto* cell = map.GetCellAt(where);
    auto* type = cell->LandType == LandType::Water ? rules.WaterCrateImg : rules.CrateImg;
    if (!type) {
        Unsorted::ScenarioInit = initialized;
        return false;
    }
    if (auto* memory = YRMemory::Allocate(sizeof(OverlayClass))) ::new (memory) OverlayClass(type, where, -1);
    cell->MarkForRedraw();
    Unsorted::ScenarioInit = initialized;
    Location = where;
    const double minimum = rules.CrateRegen * 450.0, maximum = rules.CrateRegen * 1800.0;
    const int random = ScenarioClass::Instance->Random.RandomRanged(0, 0x7FFFFFFE);
    CrateTimer.Start(game::x87_integer((maximum - minimum) * (double(random) * 4.656612877414201e-10) + minimum));
    CrateTimer.Resume();
    return true;
}
bool MapClass::PlacePowerupCrate(CellStruct cell, Powerup type) {
    const auto speed =
        GetCellAt(cell)->LandType == LandType::Water ? static_cast<SpeedType>(5) : static_cast<SpeedType>(1);
    cell = NearByLocation(cell, speed, -1, static_cast<MovementZone>(0), false, 1, 1, false, false, false, true,
                          CellStruct::Empty, false, false);
    for (auto& crate : Crates) {
        if (crate.Location != CellStruct{0, 0}) continue;
        if (!crate.Put(cell)) return false;
        if (unsigned(type) != 20) GetCellAt(cell)->OverlayData = static_cast<BYTE>(type);
        return true;
    }
    return false;
}
void MapClass::RemoveCrate(const CellStruct& cell) {
    if (SessionClass::Instance.GameMode == GameMode::Campaign) {
        auto* tile = GetCellAt(cell);
        auto* overlay = OverlayTypeClass::Array.GetItemOrDefault(tile->OverlayTypeIndex);
        if (overlay && overlay->Crate) {
            tile->MarkForRedraw();
            tile->OverlayTypeIndex = -1;
            tile->OverlayData = 0;
        }
        return;
    }
    for (auto& crate : Crates)
        if (crate.Location == cell) {
            crate.Remove();
            return;
        }
}
bool MapClass::PlaceRandomCrate() {
    Crate* free = nullptr;
    for (auto& crate : Crates) if (crate.Location == CellStruct{0, 0}) { free = &crate; break; }
    if (!free) return false;
    auto& random = ScenarioClass::Instance->Random;
    const auto& bounds = MapCoordBounds;
    for (int i = 0; i < 1000; ++i) {
        const short x = short(bounds.Left + random.RandomRanged(0, bounds.Right - 1));
        const short y = short(bounds.Top + random.RandomRanged(0, bounds.Bottom - 1));
        const CellStruct origin{x, y};
        const auto speed = GetCellAt(origin)->LandType == LandType::Water ? SpeedType::Float : SpeedType::Track;
        const auto at = NearByLocation(origin, speed, -1, MovementZone::Normal, false, 1, 1, false, false, false,
                                       true, CellStruct{0, 0}, false, false);
        if (free->Put(at)) return true;
    }
    return false;
}
