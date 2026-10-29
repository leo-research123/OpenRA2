// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Mark; YR 0x6F4A70.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "techno_spatial.hpp"
bool TechnoClass::Mark(MarkType mark) {
    game::techno_spatial_dirty(this);
    if (!ObjectClass::Mark(mark)) return false;
    if (IsTether) SendToFirstLink(RadioCommand::RequestRedraw);
    return true;
}
