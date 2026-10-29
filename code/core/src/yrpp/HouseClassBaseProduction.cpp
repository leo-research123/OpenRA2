// Base production and power insertion, YR 0x004FE3E0 and 0x004FBD80.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
TechnoTypeClass* HouseClass::GetProductionType(AbstractType kind, bool) const {
    switch (kind) {
        case AbstractType::Building:
        case AbstractType::BuildingType:
            return ProducingBuildingTypeIndex == -1 ? nullptr : BuildingTypeClass::Array[ProducingBuildingTypeIndex];
        case AbstractType::Infantry:
        case AbstractType::InfantryType:
            return ProducingInfantryTypeIndex == -1 ? nullptr : InfantryTypeClass::Array[ProducingInfantryTypeIndex];
        case AbstractType::Unit:
        case AbstractType::UnitType:
            return ProducingUnitTypeIndex == -1 ? nullptr : UnitTypeClass::Array[ProducingUnitTypeIndex];
        case AbstractType::Aircraft:
        case AbstractType::AircraftType:
            return ProducingAircraftTypeIndex == -1 ? nullptr : AircraftTypeClass::Array[ProducingAircraftTypeIndex];
        default:
            return nullptr;
    }
}
int HouseClass::AI_BaseConstructionUpdate() {
    if (ProducingBuildingTypeIndex != -1 || !ConYards.Count) return 15;
    auto* node = Base.FindNextNode(-1);
    if (!node) return 15;
    if (node->BuildingTypeIndex >= 0 && BuildingTypeClass::Array[node->BuildingTypeIndex]->Naval && !NodeLogic_1F0) {
        Base.BaseNodes.RemoveItem(static_cast<int>(node - Base.BaseNodes.Items));
        node = Base.FindNextNode(-1);
    }
    if (!node) return 15;
    auto& nodes = Base.BaseNodes;
    const int nodeIndex = static_cast<int>(node - nodes.Items);
    if (node->BuildingTypeIndex == -3) {
        nodes.RemoveItem(nodeIndex);
        AddBasePerimeterWalls();
        return 1;
    }
    const BaseNodeClass saved = *node;
    if (node->BuildingTypeIndex == -1 ||
        (node->BuildingTypeIndex >= 0 && BuildingTypeClass::Array[node->BuildingTypeIndex] == RulesClass::Instance->WallTower &&
         node->MapCoords == CellStruct::Empty)) {
        if (ScenarioClass::Instance->Random.RandomRanged(0, 99) < RulesClass::Instance->AIPickWallDefensePercent[int(AIDifficulty)] &&
            AddBaseProtectionWalls(nodeIndex)) {
            nodes.RemoveItem(nodes.FindItemIndex(saved));
            return 1;
        }
        if (!ExpandBaseDefense(nodeIndex, Base.Cells_38.Count ? &Base.Cells_38 : nullptr)) {
            // WallTower failure also discards the immediately following placeholder.
            if (RulesClass::Instance->WallTower && nodes[nodeIndex].BuildingTypeIndex == RulesClass::Instance->WallTower->ArrayIndex)
                nodes.RemoveItem(nodeIndex);
            nodes.RemoveItem(nodeIndex);
            return 1;
        }
        node = &nodes[nodeIndex];
    }
    if (node->BuildingTypeIndex < 0) return 15;
    auto* type = BuildingTypeClass::Array[node->BuildingTypeIndex];
    // 0x4FE8EF: an outage/drain does not insert extra permanent power plants.
    if (type->PowerDrain > 0 && type->PowerDrain + PowerDrain > PowerOutput - PowerSurplus &&
        RulesClass::Instance->BuildConst.FindItemIndex(type) < 0 && !PowerBlackoutTimer.GetTimeLeft() &&
        !IsBeingDrained) {
        auto& rules = *RulesClass::Instance;
        auto* power = rules.GDIPowerPlant;
        if (SideIndex == 2) {
            power = rules.ThirdPowerPlant;
        } else if (SideIndex != 0) {
            DynamicVectorClass<BuildingTypeClass*> buildings;
            // 0x4FE7E0 copies the building pointers themselves, in reverse order.
            // Keep this original comparison behavior even though the predicate
            // normally consumes type pointers.
            for (int i = Buildings.Count - 1; i >= 0; --i)
                buildings.AddItem(reinterpret_cast<BuildingTypeClass*>(Buildings[i]));
            power = AllPrerequisitesAvailable(rules.NodAdvancedPower, buildings, buildings.Count)
                        ? rules.NodAdvancedPower : rules.NodRegularPower;
        }
        const int index = static_cast<int>(node - Base.BaseNodes.Items);
        const BaseNodeClass added{power->ArrayIndex, CellStruct::Empty, false, 0};
        auto& nodes = Base.BaseNodes;
        if (nodes.AddItem(added)) {
            for (int i = nodes.Count - 1; i > index; --i) nodes[i] = nodes[i - 1];
            nodes[index] = added;
        }
        return 1;
    }
    if (type->PowersUpToLevel == -1 && node->MapCoords != CellStruct::Empty && *type->PowersUpBuilding) {
        auto* existing = MapClass::Instance.GetCellAt(node->MapCoords)->GetBuilding();
        auto* parent = BuildingTypeClass::Find(type->PowersUpBuilding);
        if (!existing || existing->Type != parent ||
            (parent->PowersUpToLevel == -1 && existing->UpgradeLevel >= parent->Upgrades) ||
            (parent->PowersUpToLevel > 0 && existing->UpgradeLevel > 0))
            node->MapCoords = CellStruct::Empty;
    }
    ProducingBuildingTypeIndex = node->BuildingTypeIndex;
    return 15;
}

// 0x509700 alternates scarce credits between base construction and mobile units.
void HouseClass::AI_ProductionCompleted(AbstractType kind) {
    if (IsControlledByHuman() || SessionClass::Instance.GameMode == GameMode::Campaign) return;
    const auto& rules = *RulesClass::Instance;
    switch (AIState_1E4) {
        case 0:
            if (Available_Money() < rules.AIAlternateProductionCreditCutoff)
                AIState_1E4 = kind == AbstractType::Building ? 2 : 1;
            return;
        case 1:
            if (Available_Money() >= rules.AIAlternateProductionCreditCutoff)
                AIState_1E4 = 0;
            else if (kind == AbstractType::Building)
                AIState_1E4 = 2;
            return;
        case 2: {
            if (Available_Money() >= rules.AIAlternateProductionCreditCutoff) {
                AIState_1E4 = 0;
                return;
            }
            bool barracks = false, weapons = false;
            for (auto* type : rules.BuildBarracks)
                if (!barracks && ActiveBuildingTypes.GetItemCount(type->ArrayIndex) > 0) barracks = true;
            for (auto* type : rules.BuildWeapons)
                if (!weapons && ActiveBuildingTypes.GetItemCount(type->ArrayIndex) > 0) weapons = true;
            if (weapons && barracks && PowerDrain <= PowerOutput &&
                ScenarioClass::Instance->Random.RandomRanged(0, 1)) return;
            if (kind != AbstractType::Building) AIState_1E4 = 1;
            return;
        }
        case 3:
            if (Available_Money() > rules.AIAlternateProductionCreditCutoff) AIState_1E4 = 0;
            return;
        default: return;
    }
}
