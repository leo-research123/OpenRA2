// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
#include "terrain_render_rect.hpp"
#include "yrpp/TerrainClass.h"
#include "yrpp/TacticalClass.h"

namespace game {
// OpenTS terrain.cpp Get_Render_Rect; YR 0x0071D160. Union keeps the original
// +1 extension only when the second rectangle extends the right/bottom edge.
RectangleStruct terrain_render_rect(const TerrainClass& terrain) {
    RectangleStruct result;
    RectangleStruct* output = &result;
    auto* image = terrain.GetImage();
    if (!image || !TacticalClass::Instance) {
        *output = {};
        return result;
    }
    if (auto* ref = image->AsReference()) {
        ref->Load();
        image = ref->Data;
    }
    if (!image || image->Frames <= 0) {
        *output = {};
        return result;
    }
    auto a = image->GetFrameBounds(0), b = image->GetFrameBounds(image->Frames / 2);
    if (a.Width <= 0 || a.Height <= 0)
        a = b;
    else if (b.Width > 0 && b.Height > 0) {
        if (a.X > b.X) {
            a.Width += a.X - b.X;
            a.X = b.X;
        }
        if (a.Y > b.Y) {
            a.Height += a.Y - b.Y;
            a.Y = b.Y;
        }
        if (a.X + a.Width < b.X + b.Width) a.Width = b.X + b.Width - a.X + 1;
        if (a.Y + a.Height < b.Y + b.Height) a.Height = b.Y + b.Height - a.Y + 1;
    }
    a.X += terrain.unknown_rect_D0.Width - TacticalClass::Instance->TacticalPos.X - image->Width / 2;
    a.Y += terrain.unknown_rect_D0.Height - TacticalClass::Instance->TacticalPos.Y - image->Height / 2;
    *output = a;
    return result;
}
}
