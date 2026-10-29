// Original carrier launch/reload/regeneration state machine, YR 0x6B7230.
#include "yrpp/SpawnManagerClass.h"
#include "techno_spatial.hpp"
#include "yrpp/Kamikaze.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/AnimClass.h"
#include "com_error.hpp"
#include <cstdlib>

void SpawnManagerClass::Update() {
    if (UpdateTimer.GetTimeLeft()) return;
    UpdateTimer.Start(10);
    const auto& rules = *RulesClass::Instance;
    for (int i = 0; i < SpawnedNodes.Count; ++i) {
        auto* node = SpawnedNodes[i];
        auto* unit = node->Unit;
        const auto return_to_owner = [&] {
            unit->SetDestination(Owner, true);
            unit->SetTarget(nullptr);
            unit->QueueMission(Mission::Move, false);
            node->Status = SpawnNodeStatus::Returning;
        };
        switch (node->Status) {
            case SpawnNodeStatus::Idle: {
                if (!Target || SpawnTimer.GetTimeLeft() || Status == SpawnManagerStatus::CoolDown) break;
                if (node->IsSpawnMissile == 1) {
                    auto* foot = static_cast<FootClass*>(Owner);
                    if (!foot->Locomotor) game::raise_com_error(static_cast<std::int32_t>(0x80004003u));
                    if (foot->Locomotor->Is_Moving()) break;
                    if (!foot->Locomotor) game::raise_com_error(static_cast<std::int32_t>(0x80004003u));
                    if (foot->Locomotor->Is_Moving_Now()) break;
                }
                if ((Owner->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None &&
                    static_cast<FootClass*>(Owner)->IsAttackedByLocomotor)
                    break;
                SpawnTimer.Start(Owner->GetTechnoType()->MissileSpawn ? 9 : 20);
                bool reset_burst = false;
                if (node->IsSpawnMissile == 1) {
                    auto* weapon = Owner->GetWeapon(0)->WeaponType;
                    if (weapon && weapon->Burst > 1) {
                        reset_burst = true;
                        Owner->CurrentBurstIndex = i & 1;
                    }
                }
                node->Status = SpawnNodeStatus::Preparing;
                const int weapon = Owner->GetWeapon(0)->WeaponType->Spawner ? 0 : 1;
                CoordStruct launch;
                Owner->GetFLH(&launch, weapon,
                              Owner->CurrentBurstIndex ? Owner->GetTechnoType()->SecondSpawnOffset
                                                       : CoordStruct::Empty);
                launch.Z += 10;
                if (SpawnType == rules.CMisl.Type) {
                    launch.X -= 40;
                    launch.Y -= 40;
                }
                unit->Unlimbo(launch, static_cast<DirType>(Owner->PrimaryFacing.Current().GetValue<8>()));
                if (SpawnType == rules.CMisl.Type) {
                    if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
                        ::new (memory)
                            AnimClass(AnimTypeClass::Find("V3TAKOFF"), unit->Location, 2, 1, 0x600, -10, false);
                }
                if (reset_burst) Owner->CurrentBurstIndex = 0;
                if (node->IsSpawnMissile == 1) {
                    UpdateTarget();
                    unit->SetDestination(Target, true);
                } else
                    unit->SetDestination(
                        MapClass::Instance.GetCellAt(Owner->GetMapCoords())->GetNeighbourCell(FacingType::North), true);
                unit->QueueMission(Mission::Move, false);
                break;
            }
            case SpawnNodeStatus::TakeOff:
                if (!node->SpawnTimer.GetTimeLeft()) UnlinkPointer(unit);
                break;
            case SpawnNodeStatus::Preparing:
                if (node->IsSpawnMissile != 1) {
                    UpdateTarget();
                    if (!Target)
                        return_to_owner();
                    else {
                        unit->SetDestination(
                            MapClass::Instance.GetCellAt(Owner->GetMapCoords())->GetNeighbourCell(FacingType::South),
                            true);
                        unit->QueueMission(Mission::Move, false);
                    }
                }
                break;
            case SpawnNodeStatus::Attacking:
                UpdateTarget();
                if (unit->Ammo && Target) {
                    unit->SetTarget(Target);
                    unit->QueueMission(Mission::Attack, false);
                } else
                    return_to_owner();
                break;
            case SpawnNodeStatus::Returning:
                UpdateTarget();
                if (unit->Ammo > 0 && Target) {
                    node->Status = SpawnNodeStatus::Attacking;
                    unit->SetTarget(Target);
                    unit->QueueMission(Mission::Attack, false);
                } else if (unit->GetMapCoords() == Owner->GetMapCoords() && unit->Location.Z - Owner->Location.Z < 20) {
                    unit->Limbo();
                    node->Status = SpawnNodeStatus::Reloading;
                    node->SpawnTimer.Start(ReloadRate);
                } else
                    return_to_owner();
                break;
            case SpawnNodeStatus::Reloading:
                if (!node->SpawnTimer.GetTimeLeft()) {
                    node->Status = SpawnNodeStatus::Idle;
                    unit->Ammo = unit->Type->Ammo;
                    unit->Health = unit->EstimatedHealth = unit->Type->Strength;
                    game::techno_spatial_dirty(unit);
                }
                break;
            case SpawnNodeStatus::Dead:
                if (!node->SpawnTimer.GetTimeLeft()) {
                    node->Unit = static_cast<AircraftClass*>(SpawnType->CreateObject(Owner->GetOwningHouse()));
                    node->IsSpawnMissile = SpawnType == rules.V3Rocket.Type || SpawnType == rules.DMisl.Type ||
                                           SpawnType == rules.CMisl.Type;
                    if (!node->Unit) std::abort();
                    node->Unit->Limbo();
                    node->Unit->SpawnOwner = Owner;
                    node->Status = SpawnNodeStatus::Idle;
                }
                break;
            default:
                break;
        }
    }
    switch (Status) {
        case SpawnManagerStatus::Idle:
            UpdateTarget();
            if (Target) {
                if (Owner->IsCloseEnoughToAttack(Target))
                    Status = SpawnManagerStatus::Launching;
                else
                    ResetTarget();
            }
            break;
        case SpawnManagerStatus::Launching: {
            if (!Target) {
                ResetTarget();
                break;
            }
            for (int i = 0; i < SpawnedNodes.Count; ++i)
                if (SpawnedNodes[i]->Status != SpawnNodeStatus::Preparing &&
                    SpawnedNodes[i]->Status != SpawnNodeStatus::Dead)
                    return;
            bool launched_missile = false;
            for (int i = 0; i < SpawnedNodes.Count; ++i) {
                auto* node = SpawnedNodes[i];
                if (node->Status != SpawnNodeStatus::Preparing) continue;
                if (node->Unit->Type->MissileSpawn) {
                    launched_missile = true;
                    Kamikaze::Instance.Add(node->Unit, Target);
                    Kamikaze::Instance.UpdateTimer.Start(2);
                    if (node->IsSpawnMissile == 1) {
                        node->Status = SpawnNodeStatus::TakeOff;
                        const auto& rocket = SpawnType == rules.V3Rocket.Type ? rules.V3Rocket : rules.DMisl;
                        node->SpawnTimer.Start(rocket.PauseFrames + rocket.TiltFrames);
                    } else
                        UnlinkPointer(node->Unit);
                } else {
                    node->Status = SpawnNodeStatus::Attacking;
                    node->Unit->SetTarget(Target);
                    node->Unit->QueueMission(Mission::Attack, false);
                }
            }
            if (launched_missile) ResetTarget();
            Status = SpawnManagerStatus::CoolDown;
            break;
        }
        case SpawnManagerStatus::CoolDown:
            for (int i = 0; i < SpawnedNodes.Count; ++i)
                if (SpawnedNodes[i]->Status == SpawnNodeStatus::Attacking ||
                    SpawnedNodes[i]->Status == SpawnNodeStatus::Returning)
                    return;
            Status = SpawnManagerStatus::Idle;
            break;
    }
}
