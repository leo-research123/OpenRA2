// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::Can_Reach, calibrated against YR 0x004D9C60.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TeamClass.h"
#include <bit>
#include <cstdlib>

bool FootClass::vt_entry_320() const {
    return IsInPlayfield && (GetTechnoType()->IsTrain || IsALoaner || GetCurrentMission() == Mission::Retreat
        || (Team && Team->IsLeavingMapNow()));
}

double FootClass::ThreatAvoidanceValue() const {
    return Team && Team->Type->AvoidThreats ? 1.0 : ThreatAvoidanceCoefficient;
}

Move FootClass::CanReachCell(const CellClass* destination, FacingType facing,
    int& level, bool& bridge, const CellClass* source) const {
    if (!source) source = destination->GetNeighbourCell(
        static_cast<FacingType>((static_cast<unsigned>(facing) - 4u) & 7u));
    const auto flags = [](const CellClass* cell) { return static_cast<unsigned>(cell->Flags); };
    // Level is signed in the original, including the -1 caller sentinel.
    const auto height = [](const CellClass* cell) { return static_cast<signed char>(cell->Level); };
    if (facing == FacingType::None) {
        if (level == -1 && (flags(destination) & 0x100u)) level = height(destination) + 4;
        return Move::OK;
    }
    if (!source || !destination) return Move::OK;
    if (level == -1 && (flags(source) & 0x100u)) {
        level = height(source) + 4;
        if (!(flags(destination) & 0x200u)) return Move::No;
    }
    const bool from_bridge = flags(source) & 0x100u;
    const int destination_height = height(destination);
    const int difference = std::bit_cast<int>(static_cast<unsigned>(from_bridge ? height(source) : level)
        - static_cast<unsigned>(destination_height));
    switch (std::abs(static_cast<long long>(difference))) {
        case 0:
            if ((!(flags(destination) & 0x100u) || !(flags(destination) & 0x200u) || !from_bridge)
                && level != -1 && level != destination_height) return Move::No;
            break;
        case 1:
            if (difference > 0 ? !destination->SlopeIndex : !source->SlopeIndex) return Move::No;
            break;
        case 4:
            if (height(source) == destination_height - 4 && (level != destination_height || !from_bridge))
                return Move::No;
            if (destination_height != height(source) - 4) return Move::OK;
            if (!(flags(destination) & 0x100u) || !(flags(destination) & 0x200u)) return Move::No;
            bridge = true;
            break;
        default: return Move::No;
    }
    return Move::OK;
}
