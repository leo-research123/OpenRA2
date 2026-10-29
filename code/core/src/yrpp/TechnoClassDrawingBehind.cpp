// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Draw_Pre_Render; YR 0x006F60D0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include <bit>
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Drawing.h"
#include "yrpp/Surface.h"
#include <cstdlib>

void TechnoClass::DrawBehindMark(Point2D* position, RectangleStruct*) noexcept {
    try {
        if (VisualCharacter(false, nullptr) != VisualType::Normal || BehindAnim) return;
        if (RulesClass::Instance->Behind) {
            auto* storage = YRMemory::Allocate(sizeof(AnimClass));
            BehindAnim = nullptr;
            if (storage) {
                CoordStruct buffer;
                const auto at = *GetCoords(&buffer);
                BehindAnim = ::new (storage) AnimClass(RulesClass::Instance->Behind, at, 0, 1, 0x600, 0, false);
            }
            if (BehindAnim) {
                BehindAnim->SetOwnerObject(this);
                if (!GameOptionsClass::Instance.ShowHidden) BehindAnim->Invisible = true;
            }
            return;
        }

    } catch (...) {
        std::abort();
    }
}

void TechnoClass::DrawBehind(Point2D*, RectangleStruct* bounds) const { std::abort(); }
