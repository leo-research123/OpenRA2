// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp ground movement and order admission;
// YR 0x7360C0 / 0x738970 / 0x744270 / 0x7441B0 / 0x744210.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/JumpjetLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/ShipLocomotionClass.h"
#include "yrpp/HoverLocomotionClass.h"
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/AnimClass.h"
#include <bit>
#include <cstdlib>
#include <cstring>

bool UnitClass::InitializeLocomotor() noexcept {
    if (Locomotor) return true;
    if (!Type) return false;
    const bool jumpjet = !std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Jumpjet, sizeof(GUID));
    const bool teleport = !std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Teleport, sizeof(GUID));
    const bool ship = !std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Ship, sizeof(GUID));
    const bool hover = !std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Hover, sizeof(GUID));
    if (!jumpjet && !teleport && !ship && !hover &&
        std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Drive, sizeof(GUID)))
        return false;
    LocomotionClass* driver = nullptr;
    try {
        driver = jumpjet    ? static_cast<LocomotionClass*>(GameCreate<JumpjetLocomotionClass>())
                 : teleport ? static_cast<LocomotionClass*>(GameCreate<TeleportLocomotionClass>())
                 : hover    ? static_cast<LocomotionClass*>(GameCreate<HoverLocomotionClass>())
                 : ship     ? static_cast<LocomotionClass*>(GameCreate<ShipLocomotionClass>())
                            : GameCreate<DriveLocomotionClass>();
        if (!driver) return false;
        if (driver->Link_To_Object(this) < 0) {
            GameDelete(driver);
            return false;
        }
#if defined(_MSC_VER)
        Locomotor = static_cast<ILocomotion*>(driver);
#else
        driver->AddRef();
        Locomotor = driver;
#endif
        return true;
    } catch (...) {
        if (driver) GameDelete(driver);
        return false;
    }
}
// OpenTS Captured; YR 0x7463A0 preserves the follower-car chain while
// applying the same original Foot ownership transition to each vehicle.
bool UnitClass::SetOwningHouse(HouseClass* house, bool) {
    if (house == Owner) return false;
    auto* follower = FollowerCar;
    IsFollowerCar = false;
    if (follower) {
        follower->SetOwningHouse(house, true);
        FollowerCar = follower;
        follower->IsFollowerCar = true;
    }
    return FootClass::SetOwningHouse(house, true);
}
void UnitClass::MarkAllOccupationBits(const CoordStruct& at) {
    auto* cell = MapClass::Instance.GetCellAt(at);
    const bool alt =
        MapClass::Instance.GetCellFloorHeight(at) + CellClass::BridgeHeight <= at.Z && (unsigned(cell->Flags) & 0x100u);
    (alt ? cell->AltOccupationFlags : cell->OccupationFlags) |= 0x20u;
}
void UnitClass::UnmarkAllOccupationBits(const CoordStruct& at) {
    auto* cell = MapClass::Instance.GetCellAt(at);
    const bool alt = MapClass::Instance.GetCellFloorHeight(at) + CellClass::BridgeHeight <= at.Z;
    (alt ? cell->AltOccupationFlags : cell->OccupationFlags) &= ~0x20u;
}
bool UnitClass::ReadyToNextMission() const {
    if (CurrentMission == Mission::Sticky || CurrentMission == Mission::Rescue || Deploying || Undeploying || Unloading)
        return false;
    if (QueuedMission != Mission::Enter && Locomotor && Locomotor->Is_Moving_Now() && GetHeight() >= 0 &&
        GetCurrentMission() != Mission::Guard && (GetCurrentMission() != Mission::Attack || Target) && !unknown_bool_B8)
        return false;
    if (UnloadTimer.State1 || UnloadTimer.State2) return false;
    // 0x744338..0x74437D keeps factory-linked products on their exit orders.
    // In particular, an AI harvester must not admit Harvest before leaving.
    if (auto* link = GetNthLink()) {
        if (link->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(link)->Type->WeaponsFactory &&
            QueuedMission != Mission::Move && QueuedMission != Mission::Enter)
            return false;
    } else {
        // 0x74438B..0x74445D also protects the unlinked factory entrance cell.
        auto* building = MapClass::Instance.GetCellAt(Location)->GetBuilding();
        if (building && building->Type->WeaponsFactory) {
            const auto origin = CellClass::Coord2Cell(building->Location);
            const auto here = CellClass::Coord2Cell(Location);
            if (here.X == origin.X && short(here.Y - origin.Y) == 1) return false;
        }
    }
    return true;
}
void UnitClass::Update() {
    if (!IsAlive || InLimbo) return;
    if (DeathFrameCounter != -1) {
        const int previous = DeathFrameCounter;
        DeathFrameCounter = std::bit_cast<int>(unsigned(DeathFrameCounter) + 1u);
        if (previous >= Type->MaxDeathCounter) {
            Explode();
            Mark(MarkType::Up);
            UnInit();
            return;
        }
    }
    // 0x7360C0: hover drain attachment is checked before temporal progress.
    if (DrainTarget && Unsorted::CurrentFrame % 16 == 2 && GetCell()->GetBuilding() != DrainTarget) StopDrain();
    if (DrainAnim && !DrainTarget) StopDrain();
    if (TemporalTargetingMe) TemporalTargetingMe->Update();
    if (InLimbo || !IsAlive) return;
    // 0x007360C0: superweapon warps must advance the teleport locomotor
    // before the normal unit frame is suppressed by the temporal gates.
    if ((IsBeingWarpedOut() || IsWarpingIn()) && Unsorted::CurrentFrame % 24 == 0) {
        if (auto* sparkle = RulesClass::Instance->ChronoSparkle1)
            if (auto* memory = YRMemory::Allocate(sizeof(AnimClass))) {
                const CoordStruct at{std::bit_cast<int>(unsigned(Location.X) + 120u),
                                     std::bit_cast<int>(unsigned(Location.Y) + 120u), Location.Z};
                ::new (memory) AnimClass(sparkle, at, 0, 1, 0x600, 0, false);
            }
    }
    if (IsWarpingIn() || (IsBeingWarpedOut() && IsImmobilized)) {
        if (!Locomotor) std::abort();
        Locomotor->Process();
        if (!IsAlive) return;
    }
    if ((IsWarpingIn() && TemporalTargetingMe) || IsBeingWarpedOut()) {
        if (Target) SetTarget(nullptr);
        if (Destination) SetDestination(nullptr, true);
        return;
    }
    CurrentFiringFrame = std::max(std::bit_cast<int>(unsigned(CurrentFiringFrame) - 1u), -1);
    if (Type->DeploysInto && (RulesClass::Instance->BuildConst.FindItemIndex(Type->DeploysInto) >= 0) &&
        Owner->AutoBaseBuilding && !Owner->IsControlledByHuman() &&
        SessionClass::Instance.GameMode != GameMode::Campaign && !Owner->ConYards.Count &&
        CurrentMission != Mission::Hunt && CurrentMission != Mission::Unload)
        QueueMission(Mission::Hunt, false);
    if (ReadyToNextMission()) {
        if (GetCurrentMission() == Mission::None && QueuedMission == Mission::None) EnterIdleMode(false, true);
        NextMission();
    }
    FootClass::Update();
    if (Type->CanDisguise && !Type->PermaDisguise) UpdateDisguise();
    if (IsSinking) {
        auto at = Location;
        at.Z = std::bit_cast<int>(unsigned(at.Z) - 5u);
        SetLocation(at);
        if (GetHeight() < -400) {
            RegisterDestruction(nullptr);
            UnInit();
            return;
        }
        if ((Unsorted::CurrentFrame & 3) == 0) {
            auto& random = ScenarioClass::Instance->Random;
            const int y = random.RandomRanged(-170, 170), x = random.RandomRanged(-170, 170);
            const auto height = GetHeight();
            at = {std::bit_cast<int>(unsigned(Location.X) + unsigned(x)),
                  std::bit_cast<int>(unsigned(Location.Y) + unsigned(y)),
                  std::bit_cast<int>(unsigned(Location.Z) - unsigned(height))};
            if (auto* storage = YRMemory::Allocate(sizeof(AnimClass)))
                ::new (storage) AnimClass(RulesClass::Instance->Wake, at, 0, 1, 0x600, 0, false);
        }
    }
    if (!IsAlive) return;
    // 0x7365D8 clears the gathering effect after leaving Harvest. Keep this
    // before firing/rotation and the second mission admission (0x7366EF).
    if (GetCurrentMission() != Mission::Harvest) IsHarvesting = false;
    UpdateFiring();
    if (!IsAlive) return;
    UpdateRotation();
    if (ReadyToNextMission()) NextMission();
}
void UnitClass::UpdateRotation() {
    if (Target && !unknown_bool_6AF) {
        DirStruct direction;
        GetDirectionTo(&direction, Target);
        if (Type->Turret) {
            const auto* weapon = GetTurretWeapon();
            if (weapon && weapon->WeaponType && !weapon->WeaponType->OmniFire)
                SecondaryFacing.SetDesired(weapon->TurretLocked ? PrimaryFacing.Current() : direction);
        } else if (Type->SpeedType == SpeedType::Track && !Destination && !Locomotor->Is_Moving() &&
                   PrimaryFacing.Current() == direction)
            PrimaryFacing.SetDesired(direction);
    }
    if (Type->TurretSpins)
        SecondaryFacing.SetDesired(DirStruct(int((SecondaryFacing.Current().GetValue<8>() + 8) << 8)));
    else {
        unknown_bool_6AF = false;
        if (Type->Turret) {
            if (SecondaryFacing.IsRotating())
                unknown_bool_6AF = true;
            else if (!Target &&
                     Unsorted::CurrentFrame - LastFireBulletFrame >=
                         RulesClass::Instance->GuardAreaTargetingDelay + 5 &&
                     !SpawnManager && (!Type->DeployToFire || !Deployed)) {
                const auto* weapon = GetTurretWeapon();
                if (!Destination || (weapon && weapon->WeaponType && weapon->TurretLocked))
                    SecondaryFacing.SetDesired(PrimaryFacing.Current());
                else if (!IsAttackedByLocomotor) {
                    DirStruct direction;
                    GetDirectionTo(&direction, Destination);
                    SecondaryFacing.SetDesired(direction);
                }
            }
        }
    }
    if (Type->Turret) TurretIsRotating = SecondaryFacing.IsRotating();
}
bool UnitClass::EnterIdleMode(bool initial, bool resume) {
    const bool result = FootClass::EnterIdleMode(initial, resume);
    if (HaveMegaMission() || GetCurrentMission() == Mission::Wait) return result;
    if (GetCurrentMission() == Mission::Move && !Destination) {
        const auto cell = GetMapCoords();
        if (Location == CoordStruct{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0})
            SendToFirstLink(RadioCommand::NotifyUnlink);
    }
    HandleNavigationList();
    Mission order = Mission::Guard;
    if (Destination)
        order = Mission::Move;
    else if (Type->Harvester || Type->Weeder) {
        if (HasAnyLink() || GetCurrentMission() == Mission::Harvest || QueuedMission == Mission::Harvest) return result;
        const auto resource = Type->Harvester ? LandType::Tiberium : LandType::Weeds;
        if (initial || !Owner->IsControlledByHuman() || GetCell()->LandType == resource) order = Mission::Harvest;
        SetTarget(nullptr);
        SetDestination(nullptr, true);
    } else if (IsArmed()) {
        if (GetCurrentMission() == Mission::Guard || GetCurrentMission() == Mission::Area_Guard ||
            (GetCurrentMission() != Mission::None &&
             (CurrentMissionControl()->Zombie || CurrentMissionControl()->Paralyzed)))
            return result;
        if (Owner->IQLevel2 >= RulesClass::Instance->GuardArea || HasAbility(Ability::GuardArea) ||
            Type->DefaultToGuardArea)
            order = Mission::Area_Guard;
        if (Team || SpawnManager || SlaveManager) order = Mission::Guard;
    } else {
        SetTarget(nullptr);
        SetDestination(nullptr, true);
    }
    if (CurrentMission != Mission::Patrol && CurrentMission != Mission::Area_Guard &&
        CurrentMission != Mission::Unload && CurrentMission != Mission::Hunt)
        QueueMission(order, false);
    return result;
}
bool UnitClass::ObjectClickedAction(Action action, ObjectClass* target, bool ignoreForce) {
    const auto current = MouseOverObject(target, ignoreForce);
    if (action != current) {
        action = current;
        if (action == Action::Enter)
            action = Action::Move;
        else if (action == Action::Capture || action == Action::Sabotage)
            action = Action::Attack;
    }
    if (action == Action::Heal || action == Action::GRepair) action = Action::Attack;
    if (target == this && (action == Action::NoMove || action == Action::Attack)) return false;
    if (Berzerk) return false;
    return FootClass::ObjectClickedAction(action, target, ignoreForce);
}
bool UnitClass::CellClickedAction(Action action, CellStruct* cell, CellStruct* follow, bool ignoreForce) {
    if (Deployed && MouseOverCell(cell, false, ignoreForce) == Action::NoMove) return false;
    if (Berzerk) return false;
    return FootClass::CellClickedAction(action, cell, follow, ignoreForce);
}
int UnitClass::Mission_Move() {
    IsHarvesting = false;
    if (Deployed || Deploying || Undeploying) {
        QueueMission(Mission::Guard, false);
        return 1;
    }
    if (!UnloadTimer.AreStates00()) UnloadTimer.StartTimer10(Type->DeployTime);
    return FootClass::Mission_Move();
}
// YR 0x7414E0: vehicle attack destinations precede Foot's firing-cell search.
AbstractClass* UnitClass::ApproachTarget(DWORD query) {
    const bool queryOnly = static_cast<unsigned char>(query) != 0;
    bool direct = false;
    if (!Owner->IsControlledByHuman() && Target && !Destination) {
        auto* techno = (Target->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None
                           ? static_cast<TechnoClass*>(Target)
                           : nullptr;
        direct = (Type->Crusher || HasAbility(Ability::Crusher)) &&
                 DistanceFrom3D(Target) < RulesClass::Instance->Crush && techno && techno->IsCrushable(this) &&
                 (Type->AutoCrush || !Owner->IsControlledByHuman());
    }
    if (!direct) {
        if (Type->BalloonHover) {
            auto* weapon = GetWeapon(SelectWeapon(Target))->WeaponType;
            direct = weapon && weapon->Projectile && weapon->Projectile->Vertical;
        }
        if (!direct && Type->OmniCrusher && Target && (!Owner->IsHumanPlayer || !(ShouldLoseTargetNow & 0xFFu)) &&
            (Target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None)
            direct = static_cast<ObjectClass*>(Target)->IsCrushable(this);
    }
    if (direct) {
        if (!queryOnly) SetDestination(Target, true);
        return Target;
    }
    // Original fallback deliberately ignores queryOnly.
    return FootClass::ApproachTarget(0);
}

// OpenTS Assign_Destination, YR normal ground navigation entrance. The
// original clearing of Path[0] is essential when redirecting a moving tank.
void UnitClass::SetDestination(AbstractClass* target, bool immediate) {
    // YR 0x741983..0x741A7D keeps a balloon's active attack approach.
    if (Type->BalloonHover && !target && Destination && Target) {
        bool keep = Destination == Target || (Destination->WhatAmI() == AbstractType::Cell &&
                                              static_cast<CellClass*>(Destination)->FirstObject == Target);
        if (!keep) {
            const auto facing = PrimaryFacing.Current();
            DirStruct direction;
            GetDirectionTo(&direction, Target);
            const auto delta = std::bit_cast<short>(static_cast<unsigned short>(direction.Raw - facing.Raw));
            keep = std::abs(int(delta)) <= 0x4000;
        }
        if (keep) {
            if (GetCurrentMission() != Mission::Attack) ForceMission(Mission::Attack);
            return;
        }
    }
    if (target == Destination && !UnlimboingInfantry) return;
    UnlimboingInfantry = false;
    // 0x741AC3 checks the locomotor attacker at 0x2B0, not the mission.
    if (Deployed ? !LocomotorSource : (Deploying || Undeploying)) {
        AbortMotion();
        return;
    }
    if (GetCurrentMission() != Mission::Enter && QueuedMission != Mission::Enter || HasAnyLink())
        PathDirections[0] = -1;
    if (!target || immediate) unknown_abstract_array_588.Clear();
    if (Type->Harvester && target && target->WhatAmI() == AbstractType::Building &&
        GetCurrentMission() != Mission::Unload) {
        auto* refinery = static_cast<BuildingClass*>(target);
        if (Type->Dock.FindItemIndex(refinery->Type) >= 0 && refinery->HasFreeLink(this)) {
            if (SendCommand(RadioCommand::RequestLink, refinery) == RadioCommand::AnswerPositive &&
                GetCurrentMission() != Mission::Enter && GetCurrentMission() != Mission::Harvest)
                QueueMission(Mission::Enter, false);
        }
    }
    // YR 0x741970: the chrono miner drives ordinary orders, but restores its
    // underlying teleport locomotor for a clear cell at its linked refinery.
    if (Type->Teleporter && Type->Harvester && Locomotor && !IsBeingWarpedOut() && !IsAttackedByLocomotor) {
        struct HeldInterface {
            IUnknown* pointer;
            ~HeldInterface() {
                if (pointer) pointer->Release();
            }
        };
        auto* dock = GetNthLink();
        // Identify COM locomotors by the original class ID, including objects
        // constructed by the EXE rather than this module's C++ RTTI runtime.
        constexpr GUID persistID{0x109, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
        IPersist* persist = nullptr;
        GUID classID{};
        if (Locomotor->QueryInterface(persistID, reinterpret_cast<void**>(&persist)) < 0 || !persist) return;
        const HeldInterface heldPersist{persist};
        const auto identified = persist->GetClassID(&classID);
        if (identified < 0) return;
        const bool driving = !std::memcmp(&classID, &LocomotionClass::CLSIDs::Drive, sizeof(classID));
        const bool returning = dock && dock->WhatAmI() == AbstractType::Building &&
                               static_cast<BuildingClass*>(dock)->Type->DockUnload && target &&
                               target->WhatAmI() == AbstractType::Cell &&
                               !static_cast<CellClass*>(target)->FindObjectOfType(AbstractType::Unit, false);
        if (returning && driving) {
            constexpr GUID piggyID{0x92FEA800, 0xA184, 0x11D1, {0xB7, 0x0A, 0, 0xA0, 0x24, 0xDD, 0xAF, 0xD1}};
            IPiggyback* piggy = nullptr;
            const auto status = Locomotor->QueryInterface(piggyID, reinterpret_cast<void**>(&piggy));
            if (status < 0 && status != static_cast<HRESULT>(0x80004002u)) std::abort();
            const HeldInterface heldPiggy{piggy};
            if (piggy && piggy->Is_Piggybacking() && piggy->Is_Ok_To_End()) {
                // 0x742564..0x742587: keep the queried interfaces alive while
                // releasing and clearing the member before End_Piggyback.
#if defined(_MSC_VER)
                Locomotor.Release();
#else
                Locomotor->Release();
                Locomotor = nullptr;
#endif
                piggy->End_Piggyback(&Locomotor);
            } else {
                // 0x741970: finish the current drive track before restoring
                // Teleport. Retrying Enter must not restart Move_To below or
                // be discarded as an unchanged destination on the next call.
                Locomotor->Stop_Moving();
                ForceMission(Mission::None);
                QueueMission(Mission::Enter, false);
                unknown_bool_6AC = true;
                UnlimboingInfantry = true;
            }
        } else if (!returning && !driving) {
            auto* next = GameCreate<DriveLocomotionClass>();
            if (!next) return;
            next->Link_To_Object(this);
            next->Begin_Piggyback(Locomotor);
#if defined(_MSC_VER)
            Locomotor = static_cast<ILocomotion*>(next);
#else
            next->AddRef();
            Locomotor->Release();
            Locomotor = next;
#endif
        }
    }
    if (target && target->WhatAmI() == AbstractType::Building &&
        static_cast<BuildingClass*>(target)->Type->UnitRepair) {
        auto* dock = static_cast<BuildingClass*>(target);
        if (dock->HasAnyLink() && !dock->ContainsLink(this)) {
            SetArchiveTarget(dock);
            QueueUpToEnter = dock;
            target = nullptr;
        } else if (SendCommand(RadioCommand::RequestLink, dock) == RadioCommand::AnswerPositive) {
            if (SendToFirstLink(RadioCommand::RequestLoading) == RadioCommand::AnswerPositive) {
                // 0x742C7E..0x742D24 accepts the service destination before the
                // factory-exit gate, after any locomotor handoff above.
                FootClass::SetDestination(target, immediate);
                PathDirections[0] = -1;
                return;
            }
            SendToFirstLink(RadioCommand::NotifyUnlink);
            target = nullptr;
        }
    }
    // 0x742D7B..0x742E31 defers ordinary orders until the factory exit finishes.
    // The fixed exit cell remains admissible for locomotors without a drive track.
    if (auto* link = GetNthLink();
        link && link->WhatAmI() == AbstractType::Building &&
        static_cast<BuildingClass*>(link)->Type->WeaponsFactory && CurrentMission != Mission::Enter) {
        if (LocomotorSource)
            SendToFirstLink(RadioCommand::NotifyUnlink);
        else {
            const auto origin = CellClass::Coord2Cell(link->Location);
            auto* exit = MapClass::Instance.GetCellAt(CellStruct{short(origin.X + 4), short(origin.Y + 1)});
            if (target != exit) {
                NavQueue.Clear();
                unknown_abstract_array_588.Clear();
                if (target) QueueNavigationList(target);
                return;
            }
        }
    }
    // YR UnitClass::SetDestination releases a unit powered down at a service dock.
    if (Locomotor && !Locomotor->Is_Powered() && !Owner->IonSensitivesShouldBeOffline()) {
        auto* cell = GetCell();
        if (!(cell->Flags & CellFlags::BridgeHead)) {
            for (auto* object = cell->FirstObject; object; object = object->NextObject) {
                if (object != this && object->WhatAmI() == AbstractType::Building) {
                    auto* type = static_cast<BuildingClass*>(object)->Type;
                    if (type->UnitRepair || type->Bunker) {
                        Locomotor->Power_On();
                        break;
                    }
                }
            }
        }
    }
    FootClass::SetDestination(target, immediate);
}

// OpenTS Scatter, calibrated to YR 0x743A50's COM and cell-query order.
void UnitClass::Scatter(const CoordStruct& from, bool force, bool urgent) {
    if (!CanScatter()) return;
    // Hold the original IPersistStream subobject until the command finishes.
    // Query the active COM class rather than the type's configured locomotor.
    constexpr GUID persistID{0x10C, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    IPersistStream* persist = nullptr;
    if (!Locomotor) std::abort();
    const auto queried = Locomotor->QueryInterface(persistID, reinterpret_cast<void**>(&persist));
    // The original reports invalid dependencies through a COM error. Native
    // malformed-dependency policy terminates locally without a host exception.
    if (queried < 0 || !persist) std::abort();
    struct HeldPersist {
        IPersistStream* pointer;
        ~HeldPersist() noexcept { pointer->Release(); }
    } held{persist};
    GUID classID{};
    // YR uses the written class ID even when GetClassID returns a failure code.
    persist->GetClassID(&classID);
    if (!std::memcmp(&classID, &LocomotionClass::CLSIDs::Teleport, sizeof(classID))) return;
    if ((!CurrentMissionControl()->Scatter && !force) || PrimaryFacing.IsRotating() || (Destination && !urgent) ||
        Deployed || Deploying || Undeploying)
        return;
    if (!Locomotor) std::abort();
    if (!Locomotor->Is_Powered()) return;
    auto& map = MapClass::Instance;
    if (from == CoordStruct::Empty) {
        const auto origin = CellClass::Coord2Cell(GetDestination());
        const auto nearby = map.NearByLocation(origin, Type->SpeedType, -1, MovementZone::Normal, OnBridge, 1, 1, false,
                                               true, false, true, CellStruct::Empty, false, false);
        if (nearby != CellStruct::Empty) SetDestination(map.GetCellAt(nearby), true);
        return;
    }
    if (CurrentMissionControl()->Paralyzed || (WhatAmI() == AbstractType::Unit && Unloading) ||
        (Destination && (!urgent || unknown_bool_6AF)))
        return;
    auto& random = ScenarioClass::Instance->Random;
    if (Target && !force && random.RandomRanged(1, 4) != 1) return;
    const DirStruct direction =
        from == CoordStruct::Empty
            ? PrimaryFacing.Current()
            : DirStruct(
                  int((Math::atan2(double(from.Y) - Location.Y, double(Location.X) - from.X) - 1.5707963267948966) *
                      -10430.060040584269));
    const int start = (direction.GetValue<3>() + random.RandomRanged(0, 2) - 1) & 7;
    const auto origin = CellClass::Coord2Cell(GetDestination());
    // Preserve YR's ternary precedence (groundLevel + bool ? 4 : 0).
    const int height = (int(map.GetCellAt(origin)->Level) + int(IsOnBridge(nullptr)) ? 4 : 0) * Unsorted::LevelHeight;
    (void)GetDestination();
    CellStruct first = CellStruct::Empty, preferred = CellStruct::Empty;
    for (int i = 0; i < 8; ++i) {
        const auto facing = FacingType((start + i) & 7);
        const auto delta = Unsorted::AdjacentCell[int(facing)];
        const CellStruct candidate{short(origin.X + delta.X), short(origin.Y + delta.Y)};
        auto* cell = map.GetCellAt(candidate);
        if (!map.IsWithinUsableArea(candidate, true) ||
            IsCellOccupied(cell, facing, GetCellLevel(), nullptr, true) != Move::OK)
            continue;
        if (first == CellStruct::Empty) first = candidate;
        const CoordStruct position{candidate.X * 256 + 128, candidate.Y * 256 + 128, height};
        CellStruct adjusted;
        TacticalClass::AdjustCellForHeight(&adjusted, &position);
        if (adjusted == candidate && !map.GetCellAt(candidate)->ContainsBridgeEx()) {
            preferred = candidate;
            break;
        }
    }
    const auto target = preferred != CellStruct::Empty ? preferred : first;
    if (target != CellStruct::Empty) {
        QueueMission(Mission::Move, false);
        SetDestination(map.GetCellAt(target), true);
    }
}
