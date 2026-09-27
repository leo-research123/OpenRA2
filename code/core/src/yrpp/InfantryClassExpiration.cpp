// OpenTS 44fac744 infantry.cpp::Detach; YR 0x0051AA10.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
void InfantryClass::PointerExpired(AbstractClass* object, bool removed) {
    FootClass::PointerExpired(object, removed);
    if (Type == object) Type = nullptr;
}
