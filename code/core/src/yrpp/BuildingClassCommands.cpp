// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Repair / Repair_AI / Sell_Back /
// Can_Demolish / Do_MISSION_DECONSTRUCTION, calibrated to the YR entries
// declared in BuildingClass.h. Copyright Electronic Arts Inc. / OpenTS
// contributors; EA Section 7 terms: code/third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "techno_spatial.hpp"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/SessionClass.h"
#include "RulesClassReaders.hpp"
#include "map_world.hpp"
#include "type_resources.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <new>

bool BuildingClass::CanBeRepaired() const {
    return Health && Type->ClickRepairable && !IsStrange() && Type->Repairable && Health != Type->Strength;
}
bool BuildingClass::CanBeSold() const {
    if (DrainingMe || Type->Unsellable) return false;
    if (HasBuildUp && BState && GetCurrentMission() != Mission::Selling && GetCurrentMission() != Mission::Construction)
        return true;
    return Type->FirestormWall && !Owner->FirestormActive;
}
void BuildingClass::SetRepairState(int state) {
    if (state == -1)
        IsBeingRepaired = !IsBeingRepaired;
    else if (state == 0) {
        if (!IsBeingRepaired) return;
        IsBeingRepaired = false;
    } else if (state == 1) {
        if (IsBeingRepaired) return;
        IsBeingRepaired = true;
    }
    auto& rules = *RulesClass::Instance;
    int sound = rules.GenericClick;
    if (IsBeingRepaired) {
        if (Health == Type->Strength)
            sound = rules.ScoldSound;
        else {
            if (Owner->IsControlledByCurrentPlayer()) Flash(7);
            NeedsRepairs = true;
            if (Owner->IsControlledByCurrentPlayer() && !game::type_resources().audio_unavailable)
                VoxClass::Play("EVA_Repairing", -1, -1);
        }
    }
    if (Owner->IsControlledByCurrentPlayer() && !game::type_resources().audio_unavailable)
        VocClass::PlayAt(sound, Location);
    NeedsRedraw = true;
    game::map_object_changed();
}
void BuildingClass::UpdateRepair() {
    // 0x450630 decides whether to start repair/sale before ticking repair.
    const auto& rules = *RulesClass::Instance;
    if (Owner->IQLevel2 >= rules.RepairSell && GetCurrentMission() != Mission::Construction &&
        GetCurrentMission() != Mission::Selling && CanBeRepaired()) {
        if (Owner->Available_Money() < rules.CreditReserve) {
            if ((SessionClass::Instance.GameMode != GameMode::Campaign || AI_Sellable) && HasBeenAttacked &&
                Owner->IQLevel >= rules.SellBack &&
                unsigned(ScenarioClass::Instance->Random.RandomRanged(0, 50)) < unsigned(Owner->TechLevel) &&
                !AttachedTag && Type->Factory != AbstractType::BuildingType &&
                GetHealthPercentage() < rules.ConditionRed)
                Sell(1);
        } else if (!Owner->Repairing && !IsBeingRepaired &&
                   (HasBeenCaptured || ShouldRebuild || Owner->IsControlledByHuman())) {
            Owner->Repairing = true;
            SetRepairState(1);
            if (!Owner->IsControlledByHuman()) {
                const int maximum = rule_integer(Owner->RepairDelay * 1800.0);
                const int minimum = rule_integer(Owner->RepairDelay * 225.0);
                Owner->RepairTimer.Start(ScenarioClass::Instance->Random.RandomRanged(minimum, maximum));
            }
        }
    }
    if (!IsBeingRepaired) return;
    const int interval = rule_integer(RulesClass::Instance->RepairRate * 900.0);
    if (interval <= 0 || Unsorted::CurrentFrame % interval) return;
    NeedsRepairs = !NeedsRepairs;
    const int cost = Type->GetRepairStepCost(), step = Type->GetRepairStep();
    const int oldFrame = GetShapeNumber();
    if (Owner->Available_Money() < cost)
        IsBeingRepaired = false;
    else {
        Owner->TakeMoney(cost);
        Health += step;
        game::techno_spatial_dirty(this);
        EstimatedHealth += step;
        if (Health >= Type->Strength) {
            Health = EstimatedHealth = Type->Strength;
            IsBeingRepaired = false;
        }
        ToggleDamagedAnims(GetHealthPercentage() <= RulesClass::Instance->ConditionYellow);
        if (GetHealthPercentage() > RulesClass::Instance->ConditionYellow && DamageParticleSystem)
            DamageParticleSystem->UnInit();
        UpdateDamageFires();
        if (GetShapeNumber() != oldFrame) NeedsRedraw = true;
    }
    game::map_object_changed();
}
void BuildingClass::Sell(int control) {
    if (HasBuildUp) {
        const auto mission = GetCurrentMission();
        if ((control == 0 && mission != Mission::Selling) ||
            (control == 1 && (mission == Mission::Selling || C4Applied)))
            return;
        if ((control == -1 || control == 1) && mission != Mission::Selling) {
            QueueMission(Mission::Selling, false);
            NextMission();
            LogicClass::Instance.AddObject(this, false);
        }
        if (Owner->IsControlledByCurrentPlayer())
            VocClass::PlayGlobal(RulesClass::Instance->GenericClick, 0x2000, 1.0f);
    } else if (Type->ToTile) {
        // Pavement conversion is separate from the buildup/refund mission.
        (void)Type->GetActualCost(Owner);
        (void)Owner->IsControlledByHuman();
        Limbo();
        delete this;
    }
}
#if !defined(RA2_YRPP_GAME)
int BuildingClass::GetCrewCount() const {
    if (NoCrew || !Type->Crewed) return 0;
    const auto& rules = *RulesClass::Instance;
    int divisor = Owner->SideIndex == 0   ? rules.AlliedSurvivorDivisor
                  : Owner->SideIndex == 1 ? rules.SovietSurvivorDivisor
                  : Owner->SideIndex == 2 ? rules.ThirdSurvivorDivisor
                                          : 0;
    if (!divisor) return 0;
    if (HasBeenCaptured) divisor *= 2;
    return std::clamp(Type->GetRefund(Owner, false) / divisor, 1, 5);
}
#endif
int BuildingClass::Mission_Selling() {
    // TODO(BUILDING-SELL-SPECIAL): specialized
    // upgrade/passenger/kill bookkeeping and crew-selection arms.
    SetRepairState(0);
    const bool selected = IsSelected;
    const auto undeploy = [&] {
        if (!Type->UndeploysInto) return false;
        if (!Type->ConstructionYard) return true;
        const auto& session = SessionClass::Instance;
        return session.GameMode != GameMode::Campaign && ArchiveTarget && Owner->IsControlledByHuman() &&
               session.Config.MCVRedeploy && !MindControlledBy;
    };
    if (MissionStatus == 0) {
        if (undeploy()) {
            VoiceDeploy();
            // 0x44A9F7 reads DeploySound at type offset 0x56C, even for packup.
            if (GetTechnoType()->DeploySound != -1 && !game::type_resources().audio_unavailable)
                VocClass::PlayAt(GetTechnoType()->DeploySound, Location);
        }
        if (UpgradeLevel > 0) {
            const int slot = UpgradeLevel - 1;
            auto* upgrade = Upgrades[slot];
            if (upgrade) Owner->GiveMoney(upgrade->GetRefund(Owner, false));
            // Delegate turret and superweapon cleanup to the original upgrade path.
            RemoveUpgrade();
            Owner->RecheckPower = Owner->RecheckRadar = true;
            QueueMission(Mission::Guard, false);
            NextMission();
            SynchronizePowerAnimations();
            return 1;
        }
        IsReadyToCommence = false;
        SendToEachLink(RadioCommand::NotifyLeave);
        for (auto*& fire : DamageFireAnims) {
            if (fire) fire->UnInit();
            fire = nullptr;
        }
        MissionStatus = 1;
        return 1;
    } else if (MissionStatus == 1) {
        SendToEachLink(RadioCommand::NotifyUnlink);
        if (IsTether) return 1;
        if (Occupants.Count) UnloadOccupants(true, true);
        if (!ArchiveTarget || !Type->UndeploysInto) {
            const int count = GetCrewCount();
            const auto* foundation = GetFoundationData(false);
            int cells = 0;
            while (foundation && foundation[cells] != CellStruct{0x7FFF, 0x7FFF})
                ++cells;
            bool engineer = false;
            for (int i = 0; i < count && cells; ++i) {
                auto* crew = GetCrew();
                if (!crew) continue;
                if (crew->Engineer && engineer) continue;
                engineer = engineer || crew->Engineer;
                auto* infantry = new InfantryClass(crew, Owner);
                if (!infantry) continue;
#if !defined(RA2_YRPP_GAME)
                // Native Infantry construction leaves COM locomotion to its creation
                // caller; placement/occupation queries already require that driver.
                if (!infantry->InitializeLocomotor()) {
                    delete infantry;
                    continue;
                }
#endif
                const auto offset = foundation[ScenarioClass::Instance->Random.RandomRanged(0, cells - 1)];
                auto at = GetMapCoords();
                at.X += offset.X;
                at.Y += offset.Y;
                auto* cell = MapClass::Instance.TryGetCellAt(at);
                CoordStruct where{int(at.X) * 256 + 128, int(at.Y) * 256 + 164, Location.Z};
                if (cell) where = cell->FindInfantrySubposition(where, false, false, false);
                ++Unsorted::ScenarioInit;
                const bool placed = cell && where != CoordStruct::Empty && infantry->Unlimbo(where, DirType::North);
                --Unsorted::ScenarioInit;
                if (!placed)
                    delete infantry;
                else {
                    infantry->Scatter(CoordStruct::Empty, true, false);
                    infantry->QueueMission(Mission::Move, false);
                }
            }
        }
        if (Owner->IsControlledByCurrentPlayer() && !IsStrange() && !game::type_resources().audio_unavailable) {
            if (!undeploy()) VocClass::PlayAt(RulesClass::Instance->SellSound, Location);
            if (Type->PackupSound != -1) VocClass::PlayAt(Type->PackupSound, Location, &Audio7);
        }
        MissionStatus = 2;
        BeginMode(BStateType::Construction);
        IsReadyToCommence = false;
        if (selected) Select();
        return 1;
    } else if (MissionStatus == 2) {
        VocClass::UpdatePosition(&Location, &Audio7);
        if (!IsReadyToCommence) return 1;
        auto* house = Owner;
        house->RecheckTechTree = true;
        SetTarget(nullptr);
        if (undeploy()) {
            // 0x449E14 allocates before taking the building off the map. Allocation
            // returns null; Unit construction is noexcept and owns locomotor setup.
            ++Unsorted::ScenarioInit;
            void* storage = YRMemory::Allocate(sizeof(UnitClass));
            auto* unit = storage ? ::new (storage) UnitClass(Type->UndeploysInto, Owner) : nullptr;
            --Unsorted::ScenarioInit;
#if !defined(RA2_YRPP_GAME)
            if (unit && !unit->InitializeLocomotor()) {
                GameDelete(unit);
                unit = nullptr;
            }
#endif
            if (unit) {
                const double health = GetHealthPercentage();
                const int refund = GetRefund();
                auto* destination = ArchiveTarget;
                auto where = Location;
                if (Type->GetFoundationWidth() > 2 || Type->GetFoundationHeight(false) > 2) {
                    where.X = ((where.X - 1) / 256) * 256 + 128;
                    where.Y = ((where.Y - 1) / 256) * 256 + 128;
                }
                if (LightSource) LightSource->Deactivate(0);
                // Limbo notifies observers and clears their targets. Preserve the original
                // live observers before that notification, then hand them to the new unit.
                DynamicVectorClass<TechnoClass*> observers;
                for (auto* techno : TechnoClass::Array)
                    if (techno->Target && techno->Target->WhatAmI() == AbstractType::Building &&
                        techno->Target == this && techno->IsAlive && techno != this && techno != unit) {
                        // The original skips an observer when vector growth fails. Native
                        // DLLCreateArray uses std::allocator, whose failure must stay here.
                        try {
                            observers.AddItem(techno);
                        } catch (const std::bad_alloc&) {
                        }
                    }
                Limbo();
                if (unit->Unlimbo(where, static_cast<DirType>(Type->DeployFacing))) {
                    unit->Health = unit->EstimatedHealth =
                        std::max(1, game::x87_integer(health * unit->GetType()->Strength));
                    game::techno_spatial_dirty(unit);
                    if (SlaveManager) SlaveManager->SetOwner(unit);
                    unit->Group = Group;
                    unit->Veterancy = Veterancy;
                    if (destination) {
                        unit->SetDestination(destination, true);
                        unit->QueueMission(Mission::Move, false);
                    }
                    if (AttachedTag) {
                        unit->AttachTrigger(AttachedTag);
                        --AttachedTag->InstanceCount;
                        AttachedTag = nullptr;
                    }
                    unit->QueuedVoiceSoundController = QueuedVoiceSoundController;
                    unit->QueuedVoiceIndex = QueuedVoiceIndex;
                    unit->unknown_4F4 = unknown_4F4;
                    QueuedVoiceSoundController.SetEvent(nullptr, nullptr);
                    QueuedVoiceIndex = unknown_4F4 = 0xFFFFFFFFu;
                    if (selected) {
                        const bool feedback = Unsorted::MoveFeedback;
                        Unsorted::MoveFeedback = false;
                        unit->Select();
                        Unsorted::MoveFeedback = feedback;
                    }
                    for (auto* observer : observers)
                        observer->SetTarget(unit);
                } else {
                    // 0x44A16B refunds and retires the yard; it does not restore the yard
                    // or explicitly delete the unplaced Unit in this branch.
                    house->GiveMoney(refund);
                }
            } else
                house->GiveMoney(GetRefund());
            Disappear(true);
            Audio7.EndLooping();
            UnInit();
            return 1;
        }
        if (Owner->IsControlledByCurrentPlayer() && !Type->UndeploysInto && !game::type_resources().audio_unavailable)
            VoxClass::Play("EVA_StructureSold", -1, -1);
        house->GiveMoney(GetRefund());
        Limbo();
        float* stored[]{&Tiberium.Tiberium1, &Tiberium.Tiberium2, &Tiberium.Tiberium3, &Tiberium.Tiberium4};
        for (int i = 0; i < 4; ++i)
            if (*stored[i] > 0) {
                const float amount = *stored[i];
                *stored[i] = 0;
                house->GiveTiberium(float(rule_integer(amount)), i);
            }
        ActuallyPlacedOnMap = false;
        UnInit();
        house->RecheckPower = house->RecheckRadar = true;
    }
    NeedsRedraw = true;
    game::map_object_changed();
    return 1;
}
