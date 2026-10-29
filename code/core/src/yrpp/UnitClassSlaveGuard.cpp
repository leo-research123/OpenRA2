#include "yrpp/UnitClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "x87_integer.hpp"
#include <bit>

#if !defined(RA2_YRPP_GAME)
namespace {
bool wake(UnitClass& unit) {
    const int deadline = std::bit_cast<int>(unsigned(unit.CurrentMissionStartTime) +
                                            unsigned(RulesClass::Instance->SlaveMinerKickFrameDelay));
    if (!unit.SlaveManager || deadline >= Unsorted::CurrentFrame || !unit.SlaveManager->ShouldWakeUpNow()) return false;
    unit.SlaveManager->BeginScanning();
    return true;
}
int delay(UnitClass& unit) {
    return game::x87_integer(unit.CurrentMissionControl()->Rate * 900.0) +
           ScenarioClass::Instance->Random.RandomRanged(0, 2);
}
}
int UnitClass::Mission_Guard() {
    if (wake(*this)) return delay(*this);
    if (Type->Harvester || Type->Weeder) {
        bool harvest = false;
        if (Owner->IsControlledByHuman()) {
            if (Type->Teleporter) {
                for (int i = 0; i < 8; ++i) {
                    auto* b = GetCell()->GetNeighbourCell(static_cast<FacingType>(i))->GetBuilding();
                    if (b && b->Type->Refinery && b->Owner == Owner) { harvest = true; break; }
                }
                if (!harvest && GetStoragePercentage() == 1.0 && Locomotor && Locomotor->Is_Moving()) harvest = true;
            }
        } else {
            for (auto* dock : Type->Dock) {
                if (Owner->OwnedBuildingTypes.GetItemCount(dock->ArrayIndex) > 0) {
                    harvest = !Type->Harvester || !Owner->IsTiberiumShort;
                    break;
                }
            }
        }
        if (harvest) { QueueMission(Mission::Harvest, false); return 1; }
    }
    if (Type->DeploysInto && (RulesClass::Instance->BuildConst.FindItemIndex(Type->DeploysInto) >= 0) &&
        Owner->AutoBaseBuilding && !Owner->IsControlledByHuman()) {
        QueueMission(Mission::Unload, false);
        return delay(*this);
    }
    if (Type->Weeder && unknown_bool_6B8) {
        unknown_bool_6B8 = false;
        QueueMission(Mission::Harvest, false);
    }
    return FootClass::Mission_Guard();
}
int UnitClass::Mission_Hunt() {
    if (!Type->DeploysInto || ((RulesClass::Instance->BuildConst.FindItemIndex(Type->DeploysInto) < 0) &&
        !Target && !Owner->IsControlledByHuman())) return FootClass::Mission_Hunt();
    if (MissionStatus == 0) {
        if (GotoClearSpot() && TryToDeploy()) MissionStatus = 1;
    } else if (MissionStatus == 1 && !IsDeploying) MissionStatus = 0;
    return delay(*this);
}
int UnitClass::Mission_AreaGuard() {
    if (wake(*this)) return delay(*this);
    return FootClass::Mission_AreaGuard();
}
#endif
