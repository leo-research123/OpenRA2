// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp animation and random-idle methods.
// YR Do_Action 0x0051D6F0, controls 0x007EAF7C; idle addresses below.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
namespace {
struct Control {
    bool interrupt;
    unsigned char rate;
};
constexpr Control controls[42] = {
    {1, 0}, {1, 0}, {1, 6}, {1, 3}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {1, 3}, {1, 3}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {1, 3}, {1, 1}, {1, 3}, {1, 3}, {0, 1}, {0, 1}, {1, 1}, {1, 2}, {1, 1}, {1, 1}, {1, 1}, {0, 1},
    {1, 1}, {1, 1}, {1, 1}, {0, 1}, {0, 3}, {1, 1}, {0, 3}, {0, 1}, {0, 3}, {1, 4}, {1, 6}, {1, 3}, {1, 1}, {1, 1}};
}

// OpenTS Can_Fire; YR 0x51C8B0 uses the same action-control table as
// PlayAnim, not a separately maintained list of fire-capable stances.
FireError InfantryClass::GetFireError(AbstractClass* target, int index, bool checkRange) const {
    if (IsPlayingDeathSequence()) return FireError::CANT;
    if (CombatDamage(-1) < 0 &&
        (!target || target->WhatAmI() != AbstractType::Infantry ||
         static_cast<InfantryClass*>(target)->GetHealthPercentage() >= RulesClass::Instance->ConditionGreen))
        return FireError::ILLEGAL;
    if (target && target->WhatAmI() == AbstractType::Unit && GetTechnoType()->Pushy) {
        auto* linked = static_cast<UnitClass*>(target)->DirectRockerLinkedUnit;
        if (linked && linked != this) return FireError::ILLEGAL;
    }
    const auto error = TechnoClass::GetFireError(target, index, checkRange);
    if (error != FireError::OK) return error;
    if (SpeedPercentage > 0.1) return FireError::MOVING;
    const auto sequence = static_cast<int>(SequenceAnim);
    if (Destination && sequence != -1 && !controls[sequence].interrupt) return FireError::MOVING;
    if (Type->JumpJet) {
        constexpr GUID iid{0x109, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
        struct Held {
            IPersist* pointer = nullptr;
            ~Held() {
                if (pointer) pointer->Release();
            }
        } persist;
        if (!Locomotor) std::abort();
        const auto result = Locomotor->QueryInterface(iid, reinterpret_cast<void**>(&persist.pointer));
        if ((result < 0 && result != static_cast<HRESULT>(0x80004002u)) || !persist.pointer) std::abort();
        CLSID actual{};
        persist.pointer->GetClassID(&actual);
        const bool jumpjet = !std::memcmp(&actual, &LocomotionClass::CLSIDs::Jumpjet, sizeof(actual));
        if (jumpjet && Type->JumpJetTurn && Locomotor->Is_Moving_Now()) return FireError::MOVING;
    }
    if (auto* weapon = GetWeapon(index)->WeaponType) {
        if (weapon->UseFireParticles && Destination) return FireError::MOVING;
        if (weapon->AreaFire && target != GetCell()) return FireError::ILLEGAL;
        if (weapon->Warhead && weapon->Warhead->IvanBomb && target &&
            (target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None &&
            static_cast<ObjectClass*>(target)->AttachedBomb)
            return FireError::ILLEGAL;
    }
    if (!Locomotor) std::abort();
    return Locomotor->Can_Fire();
}

// OpenTS Is_Ready_To_Random_Animate / Random_Animate / Do_Idle;
// YR 0x5216D0 / 0x51CDB0 / 0x521C60. Tread is eligible in YR;
// deployed stances are not. Neither target selection nor movement orders are
// part of an ordinary GI's random idle action.
bool InfantryClass::IsItTimeForIdleActionYet() const {
    if (!TechnoClass::IsItTimeForIdleActionYet()) return false;
    if (!Locomotor) std::abort();
    if (Locomotor->Is_Moving() || Crawling || IsFiring) return false;
    const auto action = static_cast<unsigned>(SequenceAnim);
    return action < 2 || SequenceAnim == Sequence::Tread;
}

void InfantryClass::PlayIdleAnim(int which) { PlayAnim(which == 1 ? Sequence::Idle2 : Sequence::Idle1); }

bool InfantryClass::UpdateIdleAction() {
    if (!IsItTimeForIdleActionYet()) return false;
    auto& random = ScenarioClass::Instance->Random;
    const double minimum = RulesClass::Instance->IdleActionFrequency * 450.0;
    const double maximum = RulesClass::Instance->IdleActionFrequency * 1800.0;
    // This is Random_Double's original [0, 2147483646] draw, not the
    // differently bounded YRpp Randomizer::RandomDouble convenience helper.
    const int sample = random.RandomRanged(0, 2147483646);
    IdleActionTimer.Start(
        static_cast<int>((maximum - minimum) * (static_cast<double>(sample) * 4.656612877414201e-10) + minimum));
    if (Type->Fraidycat && !Owner->IsControlledByHuman() && static_cast<int>(PanicDurationLeft) > 50) {
        Scatter(CoordStruct::Empty, true, false);
        return true;
    }
    int choice = random.RandomRanged(0, 10);
    const bool cow = !_strcmpi(Type->ID, "COW");
    if (cow && random.RandomRanged(0, 10) < 5) choice = 8;
    const auto turn = [&] { PrimaryFacing.SetCurrent(DirStruct(random.RandomRanged(0, 7) * 0x2000)); };
    switch (choice) {
        case 1:
        case 2:
        case 7:
            PlayAnim(Sequence::Idle2);
            // Unlike OpenTS, YR does NOT turn after choosing Idle2.
            if (!IsSelected && Owner->IsControlledByCurrentPlayer() && Type->VoiceComment.Count > 0 &&
                Randomizer::Global.RandomRanged(0, 2) == 0)
                VocClass::PlayAt(Type->VoiceComment[0], Location);
            break;
        case 3:
        case 4:
        case 5:
            PlayAnim(Sequence::Idle1);
            break;
        case 6:
        case 9:
        case 10:
            turn();
            break;
        case 8:
            turn();
            if (cow || (!Owner->IsControlledByHuman() && Type->Fraidycat)) Scatter(CoordStruct::Empty, true, false);
            break;
        default:
            break;
    }
    return true;
}

bool InfantryClass::PlayAnim(Sequence requested, bool force, bool randomStart) {
    const int requested_index = static_cast<int>(requested);
    if (!Type || !Type->Sequence || requested_index < 0 || requested_index >= 42 ||
        !Type->Sequence->Sequences[requested_index].CountFrames ||
        (SequenceAnim == Sequence::Paradrop && IsFallingDown))
        return false;
    if (randomStart && !ScenarioClass::Instance) return false;  // no fabricated RNG outside a scenario
    Sequence action = requested;
    if (SlaveOwner && GetTechnoType()->Storage && GetStoragePercentage() == 1.0 && action == Sequence::Walk)
        action = Sequence::Carry;
    else if (action == Sequence::Down && !Type->Crawls)
        return false;
    if (Type->MovementZone == MovementZone::AmphibiousDestroyer) {
        const auto* cell = GetCell();
        const bool water =
            cell && (cell->LandType == LandType::Water || cell->LandType == LandType::Beach) && !OnBridge;
        if (water) switch (action) {
                case Sequence::Walk:
                case Sequence::Crawl:
                    action = Sequence::Swim;
                    break;
                case Sequence::Prone:
                case Sequence::Ready:
                    action = Sequence::Tread;
                    break;
                case Sequence::Idle1:
                    action = Sequence::WetIdle1;
                    break;
                case Sequence::Idle2:
                    action = Sequence::WetIdle2;
                    break;
                case Sequence::Die1:
                    action = Sequence::WetDie1;
                    break;
                case Sequence::Die2:
                    action = Sequence::WetDie2;
                    break;
                case Sequence::FireUp:
                case Sequence::FireProne:
                    action = Sequence::WetAttack;
                    break;
                default:
                    break;
            }
        const int state = water ? 0 : 1;
        if ((unknown_int_6E8 == 0 && state == 1) || (unknown_int_6E8 == 1 && state == 0)) {
            const int sound = water ? Type->EnterWaterSound : Type->LeaveWaterSound;
            if (sound != -1) VocClass::PlayAt(sound, Location);
        }
        unknown_int_6E8 = state;
    }
    if (IsInAir() && !OnBridge && Type->Sequence->Sequences[static_cast<int>(Sequence::Hover)].StartFrame > 0 &&
        action == Sequence::Ready)
        action = Sequence::Hover;
    else if (action == Sequence::Walk && static_cast<int>(PanicDurationLeft) >= 200)
        action = Sequence::Panic;
    const int previous = static_cast<int>(SequenceAnim);
    if (action == SequenceAnim || (previous >= 0 && previous < 42 && !force && !controls[previous].interrupt))
        return false;
    const int sound = action == Sequence::Deploy     ? Type->DeploySound
                      : action == Sequence::Undeploy ? Type->UndeploySound
                                                     : -1;
    if (sound != -1) VocClass::PlayAt(sound, Location);
    SequenceAnim = action;
    const int action_index = static_cast<int>(action);
    int rate = controls[action_index].rate;
    if (action == Sequence::Idle1 || action == Sequence::Idle2 || action == Sequence::WetIdle1 ||
        action == Sequence::WetIdle2 || action == Sequence::Hover || action == Sequence::Cheer)
        rate = GameOptionsClass::Instance.GetAnimSpeed(rate);
    Animation.Start(rate);
    const int count = Type->Sequence->Sequences[action_index].CountFrames;
    Animation.Value = randomStart ? ScenarioClass::Instance->Random.RandomRanged(0, count > 1 ? count - 1 : 0) : 0;
    if (!Health) StopMoving();
    if (action == Sequence::Down)
        Crawling = true;
    else if (action == Sequence::Up || action == Sequence::Deploy)
        Crawling = false;
    return true;
}

bool InfantryClass::ReadyToNextMission() const {
    if (CurrentMission == Mission::Sticky || CurrentMission == Mission::Rescue || IsFiring || IsFallingDown)
        return false;
    if (!Locomotor) std::abort();
    if (Locomotor->Is_Moving_Now() && GetCurrentMission() != Mission::Guard && GetCurrentMission() != Mission::Hunt &&
        (GetCurrentMission() != Mission::Attack || Target))
        return false;
    if (SequenceAnim == Sequence::Nothing) return true;
    const int sequence = static_cast<int>(SequenceAnim);
    if (sequence < 0 || sequence >= 42) std::abort();
    return controls[sequence].interrupt;
}

// OpenTS Doing_AI, calibrated to 0x520AE0. This consumes Animation.Value and
// HasChanged from the original update chain; it does not advance a host timer.
void InfantryClass::Doing_AI() {
    const auto finished = [&] { return Animation.Value >= Type->Sequence->GetSequence(SequenceAnim).CountFrames; };
    const auto moving = [&] {
        if (!Locomotor) std::abort();
        return Locomotor->Is_Moving();
    };
    if (SequenceAnim == Sequence::Nothing || finished()) {
        switch (SequenceAnim) {
            case Sequence::Die1:
            case Sequence::Die2:
            case Sequence::Die3:
            case Sequence::Die4:
            case Sequence::Die5:
                if (finished()) {
                    if (Type->DeadBodies.Count > 0 || !Type->NotHuman) {
                        auto& list = Type->DeadBodies.Count > 0 ? Type->DeadBodies : RulesClass::Instance->DeadBodies;
                        // Target allocates BEFORE consuming a random number.
                        void* storage = YRMemory::Allocate(sizeof(AnimClass));
                        if (storage) {
                            const unsigned index = static_cast<unsigned>(ScenarioClass::Instance->Random.Random()) %
                                                   static_cast<unsigned>(list.Count);
                            ::new (storage) AnimClass(list[index], GetCoords(), 0, 1, 0x600, 0, false);
                        }
                    }
                    UnInit();
                }
                break;
            case Sequence::WetDie1:
            case Sequence::WetDie2:
            case Sequence::AirDeathFinish:
                if (finished()) UnInit();
                break;
            case Sequence::Deploy:
                PlayAnim(Sequence::Deployed, true);
                if (!Type->DeployedCrushable) Uncrushable = true;
                ShortenTargetingDelay();
                break;
            case Sequence::Undeploy:
                PlayAnim(Sequence::Ready, true);
                if (!Type->DeployedCrushable) Uncrushable = false;
                break;
            case Sequence::Paradrop:
                break;
            case Sequence::AirDeathStart:
                PlayAnim(Sequence::AirDeathFalling, true);
                break;
            case Sequence::Shovel:
                PlayAnim(GetCurrentMission() == Mission::Harvest ? Sequence::Shovel : Sequence::Ready, true);
                break;
            default:
                if (SequenceAnim != Sequence::Nothing) {
                    const auto facing = Type->Sequence->GetSequence(SequenceAnim).Facing;
                    if (static_cast<int>(facing) != -1)
                        PrimaryFacing.SetCurrent(DirStruct(static_cast<int>(facing) * 0x2000));
                }
                if (moving() && SpeedPercentage > 0.1)
                    PlayAnim(Crawling ? Sequence::Crawl : Sequence::Walk, true);
                else if ((SequenceAnim == Sequence::SecondaryFire || SequenceAnim == Sequence::SecondaryProne) &&
                         Airstrike && Target && !moving()) {
                    PlayAnim(Crawling ? Sequence::SecondaryProne : Sequence::SecondaryFire, true);
                    IsFiring = false;
                } else if (SequenceAnim >= Sequence::Deploy && SequenceAnim <= Sequence::DeployedIdle)
                    PlayAnim(Sequence::Deployed, true);
                else
                    PlayAnim(Crawling ? Sequence::Prone : Sequence::Ready, true);
                break;
        }
    }
    // Original UnInit queues removal; it does not justify an early return here.
    if (SequenceAnim == Sequence::Paradrop && !IsFallingDown) PlayAnim(Sequence::Ready, true);
    const auto& sequence = Type->Sequence->GetSequence(SequenceAnim);
    for (int i = 0; i < sequence.SoundCount; ++i) {
        if (i >= 2) std::abort();  // ReadSequence admits at most two sound controls.
        if (Animation.HasChanged && Animation.Value % std::max(sequence.CountFrames, 1) ==
                                        (i ? sequence.Sound2StartFrame : sequence.Sound1StartFrame))
            VocClass::PlayAt(i ? sequence.Sound2Index : sequence.Sound1Index, Location);
    }
}
