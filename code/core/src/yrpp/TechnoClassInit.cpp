// Native TechnoClass::Init shared by every constructed techno; original 0x006F3F40.
#include "yrpp/TechnoClass.h"
#include "techno_spatial.hpp"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/TechnoTypeClass.h"
namespace {
// The original allocates through operator new and keeps a null result.
template <class T, class... Args>
T* create_or_null(Args&&... args) {
    void* memory = YRMemory::Allocate(sizeof(T));
    return memory ? ::new (memory) T(std::forward<Args>(args)...) : nullptr;
}
}
/// VA: 0x006F3F40
void TechnoClass::Init() {
    game::techno_spatial_dirty(this);
    if (Owner) IsOwnedByCurrentPlayer = HouseClass::CurrentPlayer == Owner;
    auto* type = GetTechnoType();
    if (!type) return;
    // 0x638A80 registers the token even when its allocation failed, then assigns it.
    if (!PlanningToken) {
        auto* token = create_or_null<PlanningTokenClass>(this);
        PlanningTokenClass::Array.AddItem(token);
        PlanningToken = token;
    }
    if (type->Spawns)
        SpawnManager = create_or_null<SpawnManagerClass>(this, type->Spawns, type->SpawnsNumber, type->SpawnRegenRate,
                                                         type->SpawnReloadRate);
    if (type->Enslaves)
        SlaveManager = create_or_null<SlaveManagerClass>(this, type->Enslaves, type->SlavesNumber, type->SlaveRegenRate,
                                                         type->SlaveReloadRate);
    auto* weapon = GetWeapon(0)->WeaponType;
    // The original dereferences Warhead without a check; a missing warhead is undefined there.
    auto* warhead = weapon ? weapon->Warhead : nullptr;
    if (warhead && warhead->MindControl)
        CaptureManager = create_or_null<CaptureManagerClass>(this, weapon->Damage, weapon->InfiniteMindControl);
    if (warhead && WhatAmI() != AbstractType::Building && warhead->Parasite)
        static_cast<FootClass*>(this)->ParasiteImUsing = GameCreate<ParasiteClass>(static_cast<FootClass*>(this));
    if (warhead && warhead->Temporal) TemporalImUsing = create_or_null<TemporalClass>(this);
    if (type->AirstrikeTeam > 0) Airstrike = create_or_null<AirstrikeClass>(this);
    if (type->CanDisguise && type->PermaDisguise && Owner) {
        Disguised = true;
        DisguisedAsHouse = Owner;
        const auto* rules = RulesClass::Instance;
        Disguise = Owner->SideIndex == 0   ? rules->AlliedDisguise
                   : Owner->SideIndex == 1 ? rules->SovietDisguise
                                           : rules->ThirdDisguise;
    }
    TurretRecoil.Turret = type->TurretAnimData;
    BarrelRecoil.Turret = type->BarrelAnimData;
    IsTurretRotateSoundPlaying = true;
    TurretIsRotating = false;
}
