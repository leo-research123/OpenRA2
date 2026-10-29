// Team creation and recruitment queries, calibrated to YR 0x6F09C0,
// 0x6F1870 and 0x6F18A0. Existing map members are recruited by TeamClass.
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include <new>
#include "yrpp/FootClass.h"

void TeamTypeClass::DestroyAllInstances() {
    for (int i = 0; i < TeamClass::Array.Count; ++i)
        if (TeamClass::Array[i]->Type == this) {
            GameDelete(TeamClass::Array[i]);
            --i;
        }
}
void TeamTypeClass::FlashAllInstances(int duration) {
    for (int i = 0; i < TeamClass::Array.Count; ++i)
        if (TeamClass::Array[i]->Type == this)
            for (auto* foot = TeamClass::Array[i]->FirstUnit; foot; foot = foot->NextTeamMember)
                foot->Flashing.DurationRemaining = duration;
}

TeamClass* TeamTypeClass::CreateTeam(HouseClass* house) noexcept {
    try {
        if (!house) house = GetHouse();
        if (!house) return nullptr;
        if (!Unsorted::ScenarioInit && Max >= 0) {
            int count = cntInstances;
            if (SessionClass::Instance.GameMode != GameMode::Campaign) {
                // 0x5095D0 counts this type separately for each owning house.
                count = 0;
                for (int i = TeamClass::Array.Count - 1; i >= 0; --i) {
                    auto* team = TeamClass::Array[i];
                    if (team && team->Type == this && team->Owner == house) ++count;
                }
            }
            if (count >= Max) return nullptr;
        }
        void* storage = YRMemory::Allocate(sizeof(TeamClass));
        return storage ? new (storage) TeamClass(this, house, 0) : nullptr;
    } catch (...) {
        return nullptr;
    }
}

int TeamTypeClass::GetGroup() const { return Group == -1 && TaskForce ? TaskForce->Group : Group; }

CellStruct* TeamTypeClass::GetWaypoint(CellStruct* buffer) const {
    *buffer = Waypoint == -1 ? CellStruct::Empty : ScenarioClass::Instance->GetWaypointCoords(Waypoint);
    return buffer;
}
