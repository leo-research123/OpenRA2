// Emergency economic recovery, YR 0x4FDD10.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/FactoryClass.h"
#include <bit>
#include <cstring>
#include <limits>
namespace {
int add_credits(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
UnitTypeClass* recovery_miner(const HouseClass& house) {
    const unsigned owners = 1u << (unsigned(house.Type->FindParentCountryIndex()) & 31u);
    for (auto* type : RulesClass::Instance->HarvesterUnit)
        if (type->OwnerFlags & owners) return type;
    return nullptr;
}
void insert_recovery_node(DynamicVectorClass<BaseNodeClass>& nodes, int index, int type) {
    if (nodes.Count >= nodes.Capacity) {
        if ((!nodes.IsAllocated && nodes.Capacity) || nodes.CapacityIncrement <= 0 ||
            nodes.Capacity > std::numeric_limits<int>::max() - nodes.CapacityIncrement ||
            !nodes.SetCapacity(nodes.Capacity + nodes.CapacityIncrement, nullptr))
            return;
    }
    // The original CRT memcpy at 0x7CA090 copies overlapping ranges backwards.
    if (index < nodes.Count)
        std::memmove(nodes.Items + index + 1, nodes.Items + index, (nodes.Count - index) * sizeof(BaseNodeClass));
    // 0x4FE214 / 0x4FE397 write zero coordinates.
    nodes.Items[index] = {type, {}, false, 0};
    ++nodes.Count;
}
}
bool HouseClass::AI_RecoverEconomy(int /*priority*/) noexcept {
    try {
        const bool canMine = CountResourceDestinations > 0 && CountWarfactories > 0;
        auto& rules = *RulesClass::Instance;
        const bool refinery = FirstBuildableFromArray(rules.BuildRefinery) != nullptr;
        auto* miner = recovery_miner(*this);
        bool buildMiner = false, buildWeapons = false;
        TechnoTypeClass* target = nullptr;
        if (miner && (canMine || (!refinery && CountWarfactories > 0))) {
            target = recovery_miner(*this);
            buildMiner = true;
        } else if (refinery) {
            target = FirstBuildableFromArray(rules.BuildRefinery);
        } else if (CountWarfactories == 0) {
            target = FirstBuildableFromArray(rules.BuildWeapons);
            buildWeapons = true;
        } else {
            return false;
        }
        // Valid original rules always provide the chosen recovery type.
        if (!target) return false;
        const int price = target->GetActualCost(this);
        int refund = 0;
        auto& nodes = Base.BaseNodes;
        int last = nodes.Count - 1;
        for (; last >= 0; --last) {
            auto* building = Base.GetNodeBuilding(last);
            if (building && building->CanBeSold()) {
                if (building->UpgradeLevel) {
                    auto* upgrade = building->Upgrades[int(building->UpgradeLevel) - 1];
                    Balance = add_credits(Balance, upgrade->GetRefund(this, true));
                    building->RemoveUpgrade();
                    RecheckPower = RecheckRadar = true;
                } else {
                    building->Sell(1);
                    if (building->Type->Gate) nodes.RemoveItem(last);
                    refund = add_credits(refund, building->GetRefund());
                }
            } else if (Base.IsNodeBuilt(last)) {
                SellWall(nodes[last].MapCoords, false);
                nodes.RemoveItem(last);
            }
            if (add_credits(refund, Balance) > price) break;
        }
        if (last < 0) {
            All_To_Hunt();
            return false;
        }
        // Suspend and abandon can change the live registry; reload each slot.
        for (int i = FactoryClass::Array.Count - 1; i >= 0; --i) {
            if (FactoryClass::Array[i]->Owner != this) continue;
            FactoryClass::Array[i]->Suspend(true);
            FactoryClass::Array[i]->AbandonProduction();
            delete FactoryClass::Array[i];
        }
        ProducingUnitTypeIndex = ProducingInfantryTypeIndex = ProducingAircraftTypeIndex = -1;
        ProducingBuildingTypeIndex = -1;
        if (buildMiner) {
            auto* type = recovery_miner(*this);
            if (!type) return false;
            AIState_1E4 = 2;
            ProducingUnitTypeIndex = type->ArrayIndex;
        } else {
            int index = Base.FindNextNodeIndex(-1);
            if (index == 0 || (index == -1 && nodes.Capacity < 2)) {
                All_To_Hunt();
                return false;
            }
            if (index == -1) index = 1;
            auto* type = FirstBuildableFromArray(buildWeapons ? rules.BuildWeapons : rules.BuildRefinery);
            if (!type) return false;
            const int typeIndex = type->ArrayIndex;
            insert_recovery_node(nodes, index, typeIndex);
            if (buildWeapons) {
                for (int i = nodes.Count - 1; i > index; --i) {
                    // 0x4FE266 requires BOTH equalities, even for distinct types.
                    if (nodes[i].BuildingTypeIndex == rules.BuildRefinery[0]->ArrayIndex &&
                        nodes[i].BuildingTypeIndex == typeIndex)
                        nodes.RemoveItem(i);
                }
            }
            AIState_1E4 = 1;
            AI_BaseConstructionUpdate();
        }
        if (last <= 0) All_To_Hunt();
        // The original returns false even after successfully queueing recovery.
        return false;
    } catch (...) {
        // Preserve mutations already made, but never unwind through the AI boundary.
        return false;
    }
}
