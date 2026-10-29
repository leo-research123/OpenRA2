// Original spawn-state queries, YR 0x6B7B90/0x6B7C40/0x6B7D30..0x6B7D80.
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/AircraftClass.h"

void SpawnManagerClass::SetTarget(AbstractClass* target) {
    if (target != Target) NewTarget = target;
}

bool SpawnManagerClass::UpdateTarget() {
    if (!NewTarget) return false;
    Target = NewTarget;
    NewTarget = nullptr;
    return true;
}

int SpawnManagerClass::CountAliveSpawns() const {
    int count = 0;
    for (int i = 0; i < SpawnedNodes.Count; ++i)
        if (SpawnedNodes[i]->Status != SpawnNodeStatus::Dead) ++count;
    return count;
}

int SpawnManagerClass::CountDockedSpawns() const {
    int count = 0;
    for (int i = 0; i < SpawnedNodes.Count; ++i) {
        const auto status = SpawnedNodes[i]->Status;
        if (status == SpawnNodeStatus::Idle || status == SpawnNodeStatus::Reloading) ++count;
    }
    return count;
}

int SpawnManagerClass::CountLaunchingSpawns() const {
    int count = 0;
    for (int i = 0; i < SpawnedNodes.Count; ++i) {
        const auto* node = SpawnedNodes[i];
        if (node->Status == SpawnNodeStatus::TakeOff)
            ++count;
        else if (static_cast<unsigned>(node->Status) == 2 && node->Unit && !node->Unit->InLimbo &&
                 node->Unit->GetTechnoType()->MissileSpawn)
            ++count;
    }
    return count;
}
