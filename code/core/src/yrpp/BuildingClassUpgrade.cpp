// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Can_Upgrade; YR 0x00452670.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "techno_spatial.hpp"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/HouseClass.h"
#include "map_world.hpp"
#include <cstdio>
#if !defined(RA2_YRPP_GAME)
bool BuildingClass::CanUpgrade(const BuildingTypeClass* upgrade, const HouseClass* owner) const noexcept {
    if (owner != Owner || _strcmpi(upgrade->PowersUpBuilding, Type->ID)) return false;
    const int level = upgrade->PowersUpToLevel;
    if (level == -1) {
        if (static_cast<signed char>(UpgradeLevel) < Type->Upgrades) return true;
    } else if (level <= 0 || level > 3)
        return false;
    return UpgradeLevel == 0;
}
bool BuildingClass::Upgrade() {
    if (Health != Type->Strength) {
        Health = Type->Strength;
        game::techno_spatial_dirty(this);
        if (IsDamaged) {
            IsDamaged = false;
            for (int i = 0; i < 21; ++i)
                if (Anims[i]) PlayNthAnim(static_cast<BuildingAnimSlot>(i), false, false, 0);
        }
        if (GetHealthPercentage() > RulesClass::Instance->ConditionYellow && DamageParticleSystem)
            DamageParticleSystem->UnInit();
        for (int i = 3; i <= 6; ++i)
            if (!Anims[i]) PlayNthAnim(static_cast<BuildingAnimSlot>(i), false, false, 0);
        NeedsRedraw = true;
        game::map_object_changed();
    }
    if (Type == RulesClass::Instance->NodAdvancedPower) {
        int next = int(unknown_5FC) + 1;
        if (next > 2) next = 0;
        if (next != int(unknown_5FC) && !Type->TurretAnimIsVoxel) {
            DestroyNthAnim(BuildingAnimSlot::Turret);
            char name[64];
            std::snprintf(name, sizeof(name), "%s_%c", Type->ImageFile, next + 'B');
            PlayAnim(name, BuildingAnimSlot::Turret, false, false, 0);
            unknown_5FC = DWORD(next);
        }
        ++UpgradeLevel;
        return true;
    }
    if (int(UpgradeLevel) == Type->Upgrades) return false;
    ++UpgradeLevel;
    PlayNthAnim(static_cast<BuildingAnimSlot>(int(UpgradeLevel) - 1),
                GetHealthPercentage() <= RulesClass::Instance->ConditionYellow, false, 0);
    return true;
}
// Remove the last upgrade without crediting its refund, YR 0x451690.
bool BuildingClass::RemoveUpgrade() noexcept {
    try {
        if (!UpgradeLevel) return false;
        auto* upgrade = Upgrades[int(UpgradeLevel) - 1];
        const bool superWeapon = upgrade && upgrade->SuperWeapon != -1;
        if (upgrade && upgrade->Turret) {
            DestroyNthAnim(BuildingAnimSlot::Turret);
            Upgrades[int(UpgradeLevel) - 1] = nullptr;
            UpgradeLevel = 0;
            unknown_5FC = DWORD(-1);
        } else {
            DestroyNthAnim(static_cast<BuildingAnimSlot>(int(UpgradeLevel) - 1));
            --UpgradeLevel;
            Upgrades[UpgradeLevel] = nullptr;
        }
        if (superWeapon) Owner->UpdateSuperWeapons();
        return true;
    } catch (...) {
        return false;
    }
}
#endif
