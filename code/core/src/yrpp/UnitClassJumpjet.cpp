// YR 0x746100: original INoticeSink receiver (thiscall, two DWORDs).
#include "yrpp/UnitClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/BulletClass.h"
#include "x87_integer.hpp"
#include <algorithm>
bool UnitClass::INoticeSink_Unknown(DWORD code, DWORD) {
    if ((Type->BalloonHover || Type->JumpJet) && code == 0x117B && HasAnyLink()) {
        SendToFirstLink(RadioCommand::NotifyUnloaded);
        if (!ArchiveTarget || ArchiveTarget == Destination) {
            AbortMotion();
            Scatter(CoordStruct::Empty, true, false);
        } else
            SetDestination(ArchiveTarget, true);
        return true;
    }
    if (Type->Crashable && code == 0x117C) {
        SetHeight(0);
        if (Type->BalloonHover)
            FireDeathWeapon(0);
        else
            Explode();
        UnInit();
        return true;
    }
    if (!IsAttackedByLocomotor) return false;
    const auto& rules = *RulesClass::Instance;
    const int damage = std::max(0, game::x87_integer(double(rules.CurrentStrengthDamage ? Health : Type->Strength) *
                                                     rules.FallingDamageMultiplier));
    auto* source = BeingManipulatedBy ? BeingManipulatedBy : this;
    auto* bullet = rules.DeathWeapon->Projectile->CreateBullet(this, source, damage, rules.CrushWarhead, 0, true);
    auto at = GetCoords();
    at.Z -= GetHeight();
    if (bullet) {
        bullet->Limbo();
        bullet->Detonate(at);
        bullet->Release();
    }
    if (BeingManipulatedBy)
        RegisterDestruction(BeingManipulatedBy);
    else if (ChronoWarpedByHouse)
        RegisterKill(ChronoWarpedByHouse);
    at = GetCoords();
    bullet = rules.DeathWeapon->Projectile->CreateBullet(this, source, damage, rules.C4Warhead, 0, true);
    if (bullet) {
        bullet->Limbo();
        bullet->Detonate(at);
        bullet->Release();
    }
    UnInit();
    return true;
}
