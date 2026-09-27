// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 building.cpp Mark/Unlimbo/Limbo; YR
// 0x43F180/0x440580/0x445880. Ordinary-building placement shared by map loading
// and runtime placement; wall/ToTile conversion remains outside this subset.
#include "map_world.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/SpotlightClass.h"
namespace {
void adjacency(const BuildingClass &building, int amount) {
  const auto at = building.GetMapCoords();
  const int width = building.Type->GetFoundationWidth(),
            height = building.Type->GetFoundationHeight(false);
  for (int y = -1; y <= height; ++y)
    for (int x = -1; x <= width; ++x)
      MapClass::Instance
          .GetCellAt(CellStruct{short(at.X + x), short(at.Y + y)})
          ->BlockedNeighbours += amount;
}
void clear_smudge(CellClass &cell) {
  auto *type = SmudgeTypeClass::Array.GetItemOrDefault(cell.SmudgeTypeIndex);
  if (!type || type->Width <= 0)
    return;
  const CellStruct origin{
      short(cell.MapCoords.X - cell.SmudgeData % type->Width),
      short(cell.MapCoords.Y - cell.SmudgeData / type->Width)};
  for (int y = 0; y < type->Height; ++y)
    for (int x = 0; x < type->Width; ++x)
      if (auto *occupied = MapClass::Instance.TryGetCellAt(
              CellStruct{short(origin.X + x), short(origin.Y + y)})) {
        occupied->SmudgeTypeIndex = -1;
        game::map_resource_changed(*occupied);
      }
}
} // namespace
// OpenTS Can_Enter_Cell; YR 0x00449440. The already-down mobile building
// checks one cell; ordinary placement uses the type's existing virtual slot.
Move BuildingClass::IsCellOccupied(CellClass* cell, FacingType, int, CellClass*, bool) const {
  if (!cell || !Type) return Move::No;
  const bool allowed = Type->UndeploysInto && IsOnMap
      ? MapClass::Instance.GetCellAt(cell->MapCoords)->CanThisExistHere(Type->SpeedType, Type, Owner)
      : Type->CanCreateHere(cell->MapCoords, Owner);
  return allowed ? Move::OK : Move::No;
}
bool BuildingClass::Mark(MarkType value) {
  if (!TechnoClass::Mark(value))
    return false;
  auto at = GetMapCoords();
  if (value == MarkType::Up)
    MapClass::Instance.RemoveContentAt(&at, this);
  else if (value == MarkType::Down || value == MarkType::ChangeRedraw) {
    MapClass::Instance.AddContentAt(&at, this);
    // 0x43F180 snaps again after Place_Down has recalculated foundation cells.
    CoordStruct source{at.X * 256 + 128, at.Y * 256 + 128, 0}, adjusted;
    SetLocation(*Type->vt_entry_6C(&adjusted, &source));
    for (int y = 0; y < Type->GetFoundationHeight(false); ++y)
      for (int x = 0; x < Type->GetFoundationWidth(); ++x)
        if (auto *cell = MapClass::Instance.TryGetCellAt(
                CellStruct{short(at.X + x), short(at.Y + y)}))
          clear_smudge(*cell);
  }
  return true;
}
bool BuildingClass::Unlimbo(const CoordStruct &where, DirType facing) {
  if (!Type || !Owner || !TechnoClass::Unlimbo(where, facing))
    return false;
  if (!IsAlive)
    return true;
  adjacency(*this, 1);
  Owner->RecheckPower = true;
  try {
    if (Type->LightIntensity && !LightSource) {
      LightSource = new LightSourceClass(
          GetCenterCoords(), Type->LightVisibility, Type->LightIntensity,
          {Type->LightRedTint, Type->LightGreenTint, Type->LightBlueTint});
      LightSource->Activate();
    }
    if (Type->HasSpotlight && !Spotlight)
      Spotlight = new BuildingLightClass(this);
  } catch (
      ...) { /* Optional visual allocation does not undo valid placement. */
  }
  return true;
}
bool BuildingClass::Limbo() {
  if (!InLimbo && Type && !Type->ToTile)
    adjacency(*this, -1);
  const bool result = TechnoClass::Limbo();
  if (result) {
    ActuallyPlacedOnMap = false;
    delete Spotlight;
    Spotlight = nullptr;
    delete LightSource;
    LightSource = nullptr;
    UpdateDamageFires();
  }
  return result;
}
