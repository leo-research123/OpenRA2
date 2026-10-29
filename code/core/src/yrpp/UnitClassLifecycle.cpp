// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp constructor; YR 0x7353C0 native object fields.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "techno_spatial.hpp"
#include "yrpp/LocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/EBolt.h"
#include "map_world.hpp"
#include "target_registry.hpp"
#include "com_error.hpp"
#include <cstdlib>
namespace {
DynamicVectorClass<UnitClass*> units;
}
DynamicVectorClass<UnitClass*>& UnitClass::Array = units;
UnitClass::UnitClass(UnitTypeClass* type, HouseClass* owner) noexcept
    : FootClass(owner), CurrentFiringFrame(-1), Type(type), FollowerCar(nullptr), FlagHouseIndex(-1),
      IsFollowerCar(false), Unloading(false), IsHarvesting(false), TerrainPalette(false), unknown_int_6D4(-1),
      DeathFrameCounter(-1), ElectricBolt(nullptr), Deployed(false), Deploying(false), Undeploying(false),
      NonPassengerCount(0), ToolTipText{} {
    Create_ID();
    Array.AddItem(this);
    SecondaryFacing.SetCurrent(PrimaryFacing.Current());
    if (Type) {
        ILocomotion* created = nullptr;
        const auto result = LocomotionClass::CreateInstance(&created, &Type->Locomotor, nullptr, 7);
        if (result < 0 && result != static_cast<HRESULT>(0x80004002u)) game::terminate_com_error(result);
#if defined(_MSC_VER)
        Locomotor = created;
#else
        if (created != Locomotor) {
            auto* old = Locomotor;
            Locomotor = created;
            if (created) created->AddRef();
            if (old) old->Release();
        }
#endif
        if (created) created->Release();
        if (!Locomotor) game::terminate_com_error(static_cast<HRESULT>(0x80004003u));
        Locomotor->Link_To_Object(this);
        PrimaryFacing.SetROT(Type->ROT);
        SecondaryFacing.SetROT(Type->ROT);
        Ammo = Type->InitialAmmo == -1 ? Type->Ammo : Type->InitialAmmo;
        Cloakable = Type->Cloakable;
        Health = EstimatedHealth = Type->Strength;
        game::techno_spatial_dirty(this);
    }
    ReloadTimer.Start(0);
    if (Owner) Owner->AddTracking(this);
    // 0x7355F1 calls Techno.Init even for a null type.
    Init();
    if (Type) {
        if (Owner && (Owner->Type->VeteranUnits.FindItemIndex(Type) >= 0 ||
                      (Owner->WarFactoryInfiltrated && !Type->Naval && Type->Trainable)))
            Veterancy.SetVeteran();
        // The base constructor leaves the turret unset until this gunner setup.
        if (Type->Gunner) SetCurrentWeapon(0);
    }
    game::register_target_identity(*this);
}
UnitClass::~UnitClass() {
    if (ElectricBolt) {
        ElectricBolt->ClearOwner();
        ElectricBolt = nullptr;
    }
    if (Type && Owner && CountedAsOwned) Owner->RemoveTracking(this);
    // 0x73584F calls Foot.Limbo even after Mark(Up) has cleared IsOnMap.
    // A sinking unit can still occupy Display/Logic lists until this call.
    if (Game::IsActive && Type) FootClass::Limbo();
    NotifyObjectExpired(true);
    Array.Remove(this);
    game::unregister_target_identity(*this);
    IsAlive = false;
}
void UnitClass::PointerExpired(AbstractClass* object, bool removed) {
    FootClass::PointerExpired(object, removed);
    if (FollowerCar == object) FollowerCar = nullptr;
    if (Type == object) {
        Type = nullptr;
        game::techno_spatial_dirty(this);
    }
}
