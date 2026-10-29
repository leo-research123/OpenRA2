// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp constructor/Init, YR 0x00517A50 / 0x00517CC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Actual original class and registries; native presentation is not full combat initialization.
#include "yrpp/JumpjetLocomotionClass.h"
#include "yrpp/InfantryClass.h"
#include "techno_spatial.hpp"
#include "yrpp/TeamClass.h"
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/Unsorted.h"
#include "target_registry.hpp"
#include "com_error.hpp"
#include "map_world.hpp"
#include <cstring>
#include <cstdlib>
namespace {
DynamicVectorClass<InfantryClass*> infantry;
}
DynamicVectorClass<InfantryClass*>& InfantryClass::Array = infantry;
bool InfantryClass::Limbo() {
    Locomotor->Stop_Movement_Animation();
    unknown_int_6E8 = 2;
    Crawling = false;
    SequenceAnim = Sequence::Ready;
    return FootClass::Limbo();
}
bool InfantryClass::InitializeLocomotor() noexcept {
    if (Locomotor) return true;
    if (!Type) return false;
    const bool jumpjet = !std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Jumpjet, sizeof(GUID));
    const bool teleport = !std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Teleport, sizeof(GUID));
    if (!jumpjet && !teleport && std::memcmp(&Type->Locomotor, &LocomotionClass::CLSIDs::Walk, sizeof(GUID)))
        return false;
    try {
        LocomotionClass* driver = jumpjet    ? static_cast<LocomotionClass*>(GameCreate<JumpjetLocomotionClass>())
                                  : teleport ? static_cast<LocomotionClass*>(GameCreate<TeleportLocomotionClass>())
                                             : GameCreate<WalkLocomotionClass>();
        if (!driver) return false;
        // One retained ILocomotion reference, matching the constructor's COM query.
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
        return false;
    }
}
InfantryClass::InfantryClass(InfantryTypeClass* type, HouseClass* owner) noexcept
    : FootClass(owner), Type{type}, SequenceAnim{Sequence::Nothing}, unknown_Timer_6C8{0}, PanicDurationLeft{},
      PermanentBerzerk{}, Technician{}, unknown_bool_6DA{}, Crawling{}, unknown_bool_6DC{}, unknown_bool_6DD{},
      unknown_6E0{}, ShouldDeploy{}, unknown_int_6E8{2}, unused_6EC{} {
    Create_ID();
    Array.AddItem(this);
    // Infantry.Init 0x517CC0 runs Techno.Init 0x6F3F40 first, then tracking and type defaults.
    Init();
    if (Owner) Owner->AddTracking(this);
    if (Type) {
        // The original reads Owner->Type unchecked; a typeless house is undefined there and skipped here.
        if (Owner && Owner->Type && Owner->Type->VeteranInfantry.FindItemIndex(Type) >= 0) Veterancy.SetVeteran();
        if (Owner && Owner->BarracksInfiltrated && Type->Trainable) Veterancy.SetVeteran();
        Health = EstimatedHealth = Type->Strength;
        game::techno_spatial_dirty(this);
        Ammo = Type->InitialAmmo == -1 ? Type->Ammo : Type->InitialAmmo;
        Cloakable = Type->Cloakable;
        // The constructor then creates the locomotor named by the type (0x517A50).
        ILocomotion* created = nullptr;
        LocomotionClass::CreateInstance(&created, &Type->Locomotor, nullptr, 7);
#if defined(_MSC_VER)
        Locomotor.Attach(created);
#else
        Locomotor = created;
#endif
        if (!Locomotor) game::terminate_com_error(static_cast<HRESULT>(0x80004003u));
        Locomotor->Link_To_Object(this);
    }
    PrimaryFacing.SetROT(127);
    game::register_target_identity(*this);
}
InfantryClass::~InfantryClass() {
    // 0x517D90 detaches live ownership before broadcasting expiration. In
    // particular, the manager must not retain a slave deleted before its owner.
    if (Game::IsActive && Type) {
        if (Owner && Owner->Type && Owner->CanBuild(Type, false, false) == CanBuildResult::TemporarilyUnbuildable)
            Owner->RecheckTechTree = true;
        if (Team) {
            Team->LiberateMember(this, -1, 0);
            Team = nullptr;
        }
        if (SlaveOwner && SlaveOwner->SlaveManager) SlaveOwner->SlaveManager->LostSlave(this);
        if (Owner) Owner->RemoveTracking(this);
        // Native type inspection also permits partially constructed objects without
        // a locomotor. Fully initialized game infantry always has this COM object.
        if (Locomotor) Locomotor->Stop_Movement_Animation();
        unknown_int_6E8 = 2;
        Crawling = false;
        SequenceAnim = Sequence::Ready;
        FootClass::Limbo();
    }
    NotifyObjectExpired(true);
    Array.Remove(this);
    game::unregister_target_identity(*this);
    IsAlive = false;
}
HRESULT YRPP_STDCALL InfantryClass::GetClassID(CLSID* out) {
    if (!out) return static_cast<HRESULT>(0x80004003u);
    constexpr std::uint32_t words[]{0x0E272DC4, 0x11D19C0F, 0xA00009B7, 0xD1AFDD24};
    std::memcpy(out, words, sizeof(words));
    return 0;
}
AbstractType InfantryClass::WhatAmI() const { return AbsID; }
int InfantryClass::Size() const { return sizeof(*this); }
bool InfantryClass::Unlimbo(const CoordStruct& where, DirType facing) {
    const int floor = MapClass::Instance.GetCellFloorHeight(where);
    auto at = where;
    if (at.Z == floor) {
        const bool ignoreContents =
            Unsorted::ScenarioInit || !MapClass::Instance.IsWithinUsableArea(CellClass::Coord2Cell(where), true);
        at = MapClass::Instance.GetCellAt(where)->FindInfantrySubposition(where, ignoreContents, false, false);
        if (at == CoordStruct::Empty) return false;
        at.Z = floor;
    }
    if (!FootClass::Unlimbo(at, facing)) return false;
    if (!Type->Sight) DiscoveredByCurrentPlayer = false;
    // 0xA8F234 is initialized at 0x5179B0 to four level heights.
    if (at.Z <= floor + 4 * Unsorted::LevelHeight) MarkAllOccupationBits(at);
    unknown_int_6E8 = 2;
    return true;
}
ObjectTypeClass* InfantryClass::GetType() const { return Type; }
bool InfantryClass::IsDeployed() const {
    return Type && Type->Deployer && SequenceAnim >= Sequence::Deploy && SequenceAnim <= Sequence::DeployedIdle;
}
ObjectClass* InfantryTypeClass::CreateObject(HouseClass* owner) {
    // 0x523B10 keeps a null allocation result for callers such as slave regeneration.
    void* storage = YRMemory::Allocate(sizeof(InfantryClass));
    return storage ? ::new (storage) InfantryClass(this, owner) : nullptr;
}

// OpenTS infantry.cpp Paradrop; YR 0x521760 adds the parachute sequence.
bool InfantryClass::SpawnParachuted(const CoordStruct& at) {
    if (!ObjectClass::SpawnParachuted(at)) return false;
    QueueMission(Owner->IsControlledByHuman() ? Mission::Guard : Mission::Hunt, false);
    PlayAnim(Sequence::Paradrop, true, false);
    return true;
}
