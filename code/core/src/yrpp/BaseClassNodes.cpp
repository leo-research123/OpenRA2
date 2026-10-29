// Preset campaign base-node queries, YR 0x0042E780..0x0042EBB0.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include <bit>
BuildingClass* BaseClass::GetNodeBuilding(int index) const {
    const auto& node = BaseNodes[index];
    if (node.MapCoords == CellStruct::Empty || node.BuildingTypeIndex < 0) return nullptr;
    auto* type = BuildingTypeClass::Array[node.BuildingTypeIndex];
    auto* building = MapClass::Instance.GetCellAt(node.MapCoords)->GetBuilding();
    if (!building || building->GetMapCoords() != node.MapCoords || building->Owner != Owner) return nullptr;
    if (building->Type == type) return building;
    if (!*type->PowersUpBuilding) return nullptr;
    int count = 0;
    for (auto* upgrade : building->Upgrades)
        if (upgrade == type) ++count;
    for (int i = 0; i < index; ++i)
        if (BaseNodes[i].BuildingTypeIndex == node.BuildingTypeIndex &&
            BaseNodes[i].MapCoords == CellClass::Coord2Cell(building->Location))
            --count;
    return count > 0 ? building : nullptr;
}
bool BaseClass::IsNodeBuilt(int index) const {
    if (GetNodeBuilding(index)) return true;
    const auto& node = BaseNodes[index];
    if (node.BuildingTypeIndex < 0) return false;
    auto* type = BuildingTypeClass::Array[node.BuildingTypeIndex];
    if (!type->Wall) return false;
    auto* cell = MapClass::Instance.GetCellAt(node.MapCoords);
    return (type->ToOverlay && cell->OverlayTypeIndex == type->ToOverlay->ArrayIndex) || cell->GetBuilding();
}
int BaseClass::FindNextNodeIndex(int typeIndex) const {
    for (int i = 0; i < BaseNodes.Count; ++i) {
        if ((typeIndex != -1 && BaseNodes[i].BuildingTypeIndex != typeIndex) || IsNodeBuilt(i)) continue;
        if (typeIndex == -1 && SessionClass::Instance.GameMode != GameMode::Campaign && BaseNodes[i].Placed) {
            if (!Owner->CanRebuildBaseNode(&BaseNodes[i])) continue;
            // The original const query updates the node through the vector's storage.
            BaseNodes.Items[i].MapCoords = CellStruct::Empty;
        }
        return i;
    }
    return -1;
}
BaseNodeClass* BaseClass::FindNextNode(int typeIndex) {
    const int index = FindNextNodeIndex(typeIndex);
    return index < 0 ? nullptr : &BaseNodes[index];
}
int BaseClass::FailedToPlaceNode(BaseNodeClass* node) {
    if (!node) return 0;
    return node->Attempts = std::bit_cast<int>(static_cast<unsigned>(node->Attempts) + 1u);
}
