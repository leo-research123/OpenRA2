// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::Shape_Number; normal YR 0x00518D80 branch.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include <cstdlib>
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
int InfantryClass::GetCurrentFrame() const {
    if (!Type) return -1;
    int action = static_cast<int>(SequenceAnim);
    if (action == -1) {
        auto* cell = GetCell();
        action = cell && cell->LandType == LandType::Water && !OnBridge ? 16 : 0;
    }
    if (action < 0 || action >= 42) return -1;
    auto* apparent = Type;
    if (!IsClearlyVisibleTo(HouseClass::CurrentPlayer)) {
        auto* disguise = GetDisguise(true);
        if (disguise && disguise->WhatAmI() == AbstractType::InfantryType)
            apparent = static_cast<InfantryTypeClass*>(disguise);
    }
    if (!apparent->Sequence) return -1;
    const auto& sequence = apparent->Sequence->Sequences[action];
    constexpr unsigned human[]{7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 3, 3,
                               3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 7, 7};
    std::int64_t frame = Animation.Value % std::max(sequence.CountFrames, 1);
    auto direction = PrimaryFacing.Current();
    if (Type->JumpJet && Locomotor) {
        constexpr GUID iid{0x10C, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
        IPersist* persist = nullptr;
        const auto result = Locomotor->QueryInterface(iid, reinterpret_cast<void**>(&persist));
        if (result < 0 || !persist) return -1;
        CLSID clsid{};
        persist->GetClassID(&clsid);
        persist->Release();
        if (!std::memcmp(&clsid, &LocomotionClass::CLSIDs::Jumpjet, sizeof(clsid)) && Target && !Type->JumpJetTurn)
            GetDirectionTo(&direction, Target);
    }
    if (sequence.FacingMultiplier > 0)
        frame += std::int64_t(human[direction.GetFacing<32>()]) * sequence.FacingMultiplier;
    frame += sequence.StartFrame;
    return frame >= 0 && frame <= INT32_MAX ? static_cast<int>(frame) : -1;
}

// The drawing moved to the presentation layer (presentation/builders/infantry.cpp). A call here means a
// caller bypassed the builders.
void InfantryClass::DrawIt(Point2D*, RectangleStruct*) const {
    game::record_drawing_failure(game::DrawingStatus::unsupported, "InfantryClass::DrawIt called outside the builders");
    std::abort();
}
