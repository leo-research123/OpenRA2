// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 weapon.cpp::Compute_CRC.
// YR 0x00772AE0 adds IsHouseColor, DownReport, OmniFire, DistributedWeaponFire.
// EA Section 7 terms and warranty disclaimers: third_party/opents/LICENSE.md.
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/CRC.h"
#include <cstdlib>

void WeaponTypeClass::ComputeCRC(CRCEngine& crc) const {
    try {
        AbstractTypeClass::ComputeCRC(crc);
        crc(AmbientDamage); crc(IsSonic); crc(TurboBoost); crc(Supress);
        crc(Camera); crc(IsLaser); crc(IsHouseColor); crc(Burst);
        if (Projectile) crc(Projectile->Fetch_ID());
        crc(Damage); crc(Speed);
        if (Warhead) crc(Warhead->Fetch_ID());
        crc(ROF); crc(Range);
        // Counts only: hashing list contents or additional fields changes YR sync CRC.
        crc(Report.Count); crc(DownReport.Count); crc(Anim.Count);
        crc(UseFireParticles); crc(Lobber); crc(LaserDuration); crc(IsBigLaser);
        crc(IsRailgun); crc(Charges); crc(Bright); crc(UseSparkParticles);
        crc(OmniFire); crc(DistributedWeaponFire);
    } catch (...) {
        // This void sync boundary cannot report a recoverable partial checksum.
        std::abort();
    }
}
