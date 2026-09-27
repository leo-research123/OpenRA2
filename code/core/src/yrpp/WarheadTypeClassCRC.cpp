// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 warhead.cpp::Compute_CRC, YR field order at 0x0075DEC0.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/CRC.h"
void WarheadTypeClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc);
    crc(Wall);crc(Wood);crc(Tiberium);crc(unknown_bool_149);crc(Sparky);crc(Sonic);
    crc(Fire);crc(Conventional);crc(Rocker);crc(DirectRocker);crc(Bright);
    crc(CLDisableRed);crc(CLDisableGreen);crc(CLDisableBlue);
    crc(Deform);crc(DeformTreshold);crc(ProneDamage);crc(Veinhole);
    crc(ShakeXlo);crc(ShakeXhi);crc(ShakeYlo);crc(ShakeYhi);
    crc(&Locomotor,16);
    for(double verse:Verses)crc(verse);
    crc(AnimList.Count);crc(static_cast<int>(InfDeath));
}
