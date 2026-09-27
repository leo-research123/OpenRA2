// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 bullettype.cpp::Compute_CRC; YR 0x0046C560 sequence.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletTypeClass.h"
#include "yrpp/CRC.h"
void BulletTypeClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc); // deliberately skips ObjectType CRC
    crc(Airburst);crc(Shadow);crc(Arcing);crc(Dropping);crc(Level);crc(Inviso);
    crc(Proximity);crc(Ranged);crc(NoRotate);crc(Inaccurate);crc(FlakScatter);
    crc(AA);crc(AG);crc(Degenerates);crc(Bouncy);crc(Elasticity);
    crc(Acceleration);crc(ROT);crc(Arm);crc(Flat);
}
