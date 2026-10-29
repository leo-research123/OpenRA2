// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Per_Cell_Process factory departure; YR 0x739EC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "visibility_grid.hpp"
#include "yrpp/LocomotionClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/Randomizer.h"
#include <cstdlib>
#include <cstring>
namespace {
float crush_rocking(float previous) {
    constexpr float delta = 0.02f;
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
 float result;
 __asm fld previous
 __asm fadd delta
 __asm fstp result
 return result;
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    float result;
    __asm__ volatile("flds %1; fadds %2; fstps %0" : "=m"(result) : "m"(previous), "m"(delta) : "st");
    return result;
#else
    return previous + delta;
#endif
}
}
void UnitClass::UpdatePosition(PCPType reason) {
    const auto entered = GetMapCoords();
    if ((reason == PCPType::Rotation || reason == PCPType::End) && IsDeploying) {
        TryToDeploy();
        if (!IsAlive) return;
    }
    if (reason == PCPType::End) {
        auto* link = GetNthLink();
        if (Type->PoweredUnit && !Owner->HasPoweredCenter(Type) && !GetCell()->GetBuilding()) Deactivate();
        auto* building = GetCell()->GetBuilding();
        const auto fail_enter = [&] {
            SetDestination(nullptr, true);
            QueueMission(Mission::Guard, false);
            Scatter(CoordStruct::Empty, true, true);
        };
        const auto release_control = [&] {
            SetArchiveTarget(nullptr);
            OnBridge = false;
            MissionAccumulateTime = 0;
            sub_70DE00(0);
            SetCurrentWeaponStage(0);
            if (MindControlledBy && MindControlledBy->CaptureManager) MindControlledBy->CaptureManager->FreeUnit(this);
        };
        if (GetCurrentMission() == Mission::Eaten && building && (building == Destination || building == Target)) {
            if (building->AttachedTag)
                building->AttachedTag->RaiseEvent(TriggerEvent::EnteredBy, this, CellStruct::Empty);
            const auto voice = [&](auto& sounds) {
                const unsigned selected =
                    static_cast<unsigned>(Randomizer::Global.Random()) % static_cast<unsigned>(sounds.Count);
                VocClass::PlayAt(sounds[int(selected)], Location);
            };
            if (GetTechnoType()->VoiceDie.Count > 0 && Owner->IsControlledByCurrentPlayer())
                voice(GetTechnoType()->VoiceDie);
            if (GetTechnoType()->DieSound.Count > 0) voice(GetTechnoType()->DieSound);
            building->Owner->GiveMoney(GetRefund());
            while (Passengers.FirstPassenger) {
                auto* passenger = RemoveFirstPassenger();
                if (!passenger) continue;
                while (passenger->Passengers.FirstPassenger) {
                    auto* nested = passenger->RemoveFirstPassenger();
                    if (nested) {
                        building->Owner->GiveMoney(nested->GetRefund());
                        nested->UnInit();
                    }
                }
                building->Owner->GiveMoney(passenger->GetRefund());
                passenger->UnInit();
            }
            if (ParasiteEatingMe) {
                building->Owner->GiveMoney(ParasiteEatingMe->GetRefund());
                ParasiteEatingMe->ParasiteImUsing->SuppressionTimer.Start(50);
                ParasiteEatingMe->ParasiteImUsing->ExitUnit();
            }
            if (AttachedTag) AttachedTag->RaiseEvent(TriggerEvent::DestroyedByAnything, this, CellStruct::Empty);
            if (building->Type->Grinding) {
                VocClass::PlayAt(RulesClass::Instance->EnterGrinderSound, GetCoords());
                if (building->GetAnim(BuildingAnimSlot::Active)) {
                    building->DestroyNthAnim(BuildingAnimSlot::Active);
                    building->PlayNthAnim(BuildingAnimSlot::Special,
                                          building->GetHealthPercentage() <= RulesClass::Instance->ConditionYellow,
                                          false, 0);
                }
            }
            UnInit();
            return;
        }
        if (building && building->Type->UnitAbsorb && GetCurrentMission() == Mission::Enter &&
            Destination == building) {
            if (SendCommand(RadioCommand::QueryCanEnter, building) == RadioCommand::AnswerPositive) {
                release_control();
                Limbo();
                building->Passengers.AddPassenger(this);
                Undiscover();
                if (building->Type->ExtraPowerBonus > 0) building->Owner->RecheckPower = true;
            } else
                fail_enter();
            return;
        }
        if ((GetCurrentMission() == Mission::Enter || GetCurrentMission() == Mission::Patrol) && link &&
            link->WhatAmI() == AbstractType::Building &&
            CellClass::Coord2Cell(GetCoords()) == CellClass::Coord2Cell(link->GetDockCoords(this))) {
            constexpr GUID persistID{0x109, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
            IPersist* persist = nullptr;
            CLSID id{};
            if (!Locomotor || Locomotor->QueryInterface(persistID, reinterpret_cast<void**>(&persist)) < 0 || !persist)
                std::abort();
            persist->GetClassID(&id);
            if (!std::memcmp(&id, &LocomotionClass::CLSIDs::Hover, sizeof(id)) &&
                static_cast<BuildingClass*>(link)->Type->UnitRepair && !Destination)
                Destination = link;
            if (Destination == link) {
                FootClass::UpdatePosition(reason);
                SendToFirstLink(RadioCommand::RequestCompleteEnter);
                if (!Locomotor) std::abort();
                Locomotor->Power_Off();
                persist->Release();
                return;
            }
            persist->Release();
        }
        if (IsTether && link && link->WhatAmI() == AbstractType::Building && GetCurrentMission() == Mission::Enter) {
            const CellStruct north{entered.X, short(entered.Y - 1)};
            if (MapClass::Instance.GetCellAt(north)->GetBuilding() == link) {
                const int response = int(SendCommand(RadioCommand::RequestCompleteEnter, link));
                if (response != 1 && response != 5) Scatter(CoordStruct::Empty, true, false);
            }
        }
        TechnoClass* occupant = GetCell()->GetBuilding();
        if (!occupant) occupant = GetCell()->GetAircraft(OnBridge);
        if (!occupant) occupant = GetCell()->GetUnit(OnBridge);
        if (occupant == this) {
            auto* next = NextObject;
            occupant = next && (next->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None
                           ? static_cast<TechnoClass*>(next)
                           : nullptr;
        }
        if (GetCurrentMission() == Mission::Enter && occupant && occupant != this &&
            GetMapCoords() == occupant->GetMapCoords() && ContainsLink(occupant) &&
            occupant->GetTechnoType()->Passengers > 0) {
            if (SendCommand(RadioCommand::QueryCanEnter, occupant) != RadioCommand::AnswerPositive) {
                fail_enter();
                return;
            }
            release_control();
            Limbo();
            occupant->AddPassenger(this);
            if (occupant->GetTechnoType()->OpenTopped) occupant->EnteredOpenTopped(this);
            Transporter = occupant;
            if (Type->OpenTopped) SetTargetForPassengers(nullptr);
            Undiscover();
            return;
        }
        const bool techno_destination =
            Destination && (Destination->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None;
        if (IsTether &&
            !(GetCurrentMission() == Mission::Enter && (!techno_destination || GetNthLink() == Destination)) &&
            GetCurrentMission() != Mission::Unload) {
            bool arrived = !Destination;
            if (Destination && Destination->WhatAmI() == AbstractType::Cell)
                arrived = CellClass::Coord2Cell(GetCoords()) == static_cast<CellClass*>(Destination)->MapCoords;
            if (!MapClass::Instance.GetCellAt(Location)->GetBuilding() || arrived) {
                auto* contact = GetNthLink();
                auto* factory = contact && contact->WhatAmI() == AbstractType::Building
                                    ? static_cast<BuildingClass*>(contact)
                                    : nullptr;
                if (arrived || !factory || !factory->Type->WeaponsFactory ||
                    MapClass::Instance.GetCellAt(Location)->GetBuilding() != factory) {
                    const auto response = SendToFirstLink(RadioCommand::NotifyUnloaded);
                    if (response == RadioCommand::AnswerLeave) {
                        if (Destination && Destination != GetCell()) {
                            if ((Destination->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None)
                                SendCommand(RadioCommand::RequestLoading, static_cast<TechnoClass*>(Destination));
                        } else if (Type->Harvester || Type->Weeder)
                            QueueMission(Mission::Harvest, true);
                        else if (SlaveManager)
                            SlaveManager->BeginScanning();
                        else if (!Owner->IsControlledByHuman() && factory && factory->Type->WeaponsFactory) {
                            CellStruct where;
                            Owner->WhereToGo(&where, this);
                            if (where != CellStruct::Empty) {
                                QueueMission(Mission::Move, false);
                                SetDestination(MapClass::Instance.GetCellAt(where), true);
                                NextMission();
                                SetArchiveTarget(MapClass::Instance.GetCellAt(where));
                                QueueMission(Mission::Area_Guard, false);
                            } else
                                SetArchiveTarget(nullptr);
                        } else if (ArchiveTarget && ArchiveTarget != Destination)
                            SetDestination(ArchiveTarget, true);
                        else {
                            AbortMotion();
                            Scatter(CoordStruct::Empty, true, false);
                        }
                    } else if (response != RadioCommand::AnswerNegative && (Type->Harvester || Type->Weeder)) {
                        if (ArchiveTarget) {
                            QueueMission(Mission::Harvest, false);
                            SetDestination(ArchiveTarget, true);
                            SetArchiveTarget(nullptr);
                        } else if (!Destination)
                            Scatter(CoordStruct::Empty, true, false);
                    }
                }
            }
            if (MapClass::Instance.GetCellAt(Location)->GetBuilding() && !Destination && !NavQueue.Count &&
                !unknown_abstract_array_588.Count)
                Scatter(CoordStruct::Empty, true, true);
        }
        if (GetCurrentMission() == Mission::Guard && !MapClass::Instance.CoordinatesLegal(Location) && IsInPlayfield) {
            if (Team) Team->IsLeavingMap = true;
            Stun();
            UnInit();
            return;
        }
        {
            game::SightMoveScope sight_move;
            vt_entry_48C(false, 0, false, nullptr);
            UpdateSight(false, 0, false, nullptr, 0);
        }
        auto at = Location;
        MapClass::Instance.RevealArea3(&at, 0, LastSightRange + 3, false);
        if (!Unloading && ReadyToNextMission()) NextMission();
        if ((Type->Harvester || Type->Weeder) && GetCurrentMission() != Mission::Unload &&
            CurrentMission != Mission::Enter && QueuedMission != Mission::Enter && !Unloading) {
            auto* contact = GetNthLink();
            if (contact && contact->WhatAmI() == AbstractType::Building) {
                auto* dock = static_cast<BuildingClass*>(contact);
                if (Type->Harvester && dock->Type->Refinery) SendToFirstLink(RadioCommand::NotifyUnlink);
                if (Type->Weeder && dock->Type->Weeder) SendToFirstLink(RadioCommand::NotifyUnlink);
            }
        }
        if (!Destination && PathDirections[0] == -1 && !Type->MobileFire) RearmTimer.Start(GetROF(1) / 4);
        auto* cell = MapClass::Instance.GetCellAt(Location);
        if (!Locomotor) std::abort();
        if (!Locomotor->Is_Moving() && IsCellOccupied(cell, FacingType(-1), -1, nullptr, true) == Move::No &&
            (!OnBridge || !cell->ContainsBridge()) && !IsSinking) {
            if (CurrentMission != Mission::Enter) {
                auto* warhead = RulesClass::Instance->C4Warhead;
                const auto coords = GetCoords();
                if (void* memory = YRMemory::Allocate(sizeof(AnimClass)))
                    ::new (memory) AnimClass(MapClass::SelectDamageAnimation(Health, warhead, cell->LandType, coords),
                                             Location, 0, 1, 0x2600, -15, false);
                MapClass::FlashbangWarheadAt(Health, warhead, GetCoords());
                int damage = Health;
                ReceiveDamage(&damage, 0, warhead, nullptr, true, false, nullptr);
                return;
            }
            Scatter(CoordStruct::Empty, true, true);
        }
    }
    auto* cell = MapClass::Instance.GetCellAt(entered);
    if ((Type->Crusher || HasAbility(Ability::Crusher)) && cell->OverlayTypeIndex != -1) {
        auto* overlay = OverlayTypeClass::Array[cell->OverlayTypeIndex];
        if (overlay->Crushable || (overlay->Wall && Type->MovementZone == MovementZone::CrusherAll)) {
            VocClass::PlayAt(overlay->CrushSound, GetCoords());
            cell->DamageWall(-1);
            IsCrushingSomething = false;
            RockingForwardsPerFrame = crush_rocking(RockingForwardsPerFrame);
        }
    }
    TryCrushCell(GetMapCoords(), false);
    if (IsAlive) FootClass::UpdatePosition(reason);
}
