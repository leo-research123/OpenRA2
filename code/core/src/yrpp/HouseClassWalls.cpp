// Original HouseClass::SellWall, 0x004FCE80 (also used to replace damaged walls).
#include "yrpp/HouseClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/VocClass.h"
#include "type_resources.hpp"
#include "map_world.hpp"

void HouseClass::SellWall(CellStruct& at, bool skipSound) {
    if (at == CellStruct::Empty) return;
    auto& map = MapClass::Instance;
    auto* cell = map.GetCellAt(at);
    auto* overlay = OverlayTypeClass::Array.GetItemOrDefault(cell->OverlayTypeIndex);
    auto* owner = Array.GetItemOrDefault(cell->WallOwnerIndex);
    if (!overlay || !owner || !owner->IsControlledByHuman() || !overlay->Wall) return;
    BuildingTypeClass* type = nullptr;
    for (auto* candidate : BuildingTypeClass::Array)
        if (candidate->ToOverlay == overlay) {
            type = candidate;
            break;
        }
    if (!type || type->Unsellable) return;
    if (this == CurrentPlayer && !skipSound && !game::type_resources().audio_unavailable)
        VocClass::PlayGlobal(RulesClass::Instance->SellSound, 0x2000, 1.0f, nullptr);
    // 0x4FCFA2 queries cost but does not credit a refund or decrement adjacency.
    type->GetActualCost(this);
    cell->OverlayTypeIndex = -1;
    cell->OverlayData = 0;
    cell->WallOwnerIndex = -1;
    cell->RecalcAttributes(-1);
    cell->UpdateWall();
    RadarClass::Instance.RadarBackground(at);
    TechnoClass::RemoveFromTargeting(cell);
    map.ResetZones(at);
    map.RecalculateSubZones(at);
    game::map_resource_changed(*cell);
}
