#include <cstdlib>
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp: Draw_Health_Bar; YR 0x006F64A0 positions/frames.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Building and infantry bars. Other pip groups remain explicit dependencies.
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "RulesClassReaders.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <vector>

#if !defined(RA2_YRPP_GAME)
// YR 0x00708C30; Unit's 0x00740E50 entry forwards to this method.
int TechnoClass::GetPipFillLevel() const {
    double amount, capacity;
    switch (GetTechnoType()->PipScale) {
        case PipScale::Ammo:
            amount = Ammo;
            capacity = GetTechnoType()->GetPipMax();
            break;
        case PipScale::Tiberium:
            amount = Tiberium.GetTotalAmount();
            capacity = GetTechnoType()->Storage;
            break;
        case PipScale::Passengers:
            amount = Passengers.NumPassengers;
            capacity = GetTechnoType()->GetPipMax();
            break;
        case PipScale::MindControl:
            if (!CaptureManager) return 0;
            amount = CaptureManager->GetControlledCount();
            capacity = GetTechnoType()->GetPipMax();
            break;
        default:
            return 0;
    }
    // Keep the original division, scaling, and _ftol low-word semantics even
    // for zero capacity or an out-of-range result. Do not clamp to capacity.
    const int maximum = GetTechnoType()->GetPipMax();
    const int filled = game::x87_integer(double(maximum) * (amount / capacity) + 0.5);
    return filled ? filled : amount > 0.0;
}
#endif

void TechnoClass::DrawHealthBar(Point2D* location, RectangleStruct* bounds, bool) const { std::abort(); }

void TechnoClass::DrawPipScalePips(Point2D* location, Point2D*, RectangleStruct* bounds) const { std::abort(); }
