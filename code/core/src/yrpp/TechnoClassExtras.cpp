#include <cstdlib>
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp; Copyright 2025 Electronic Arts Inc.;
// Copyright 2026 OpenTS contributors; EA terms: third_party/opents/LICENSE.md.
// Original TechnoClass::DrawExtras 0x006F5190, building and ordinary infantry display.
// Wrench uses its own animation palette and remains visible when unselected.
#include "yrpp/TechnoClass.h"
#include "bitfont_drawing.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/StringTable.h"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <bit>
#include <cwchar>


void TechnoClass::DrawExtras(Point2D* point, RectangleStruct* bounds) const { std::abort(); }

// YR 0x70A990: PIPS.SHP veteran/elite/negative-rank markers.
void TechnoClass::DrawVeterancyPips(Point2D* point, RectangleStruct* bounds) const { std::abort(); }

// OpenTS 44fac744 techno.cpp::Draw_Text_Overlay, calibrated to 0x70AA60.
// YR suppresses power text on primary factories and uses an owner-colored box.
// The native host submits the original BitFont glyphs through its own backend;
// the DDraw text wrapper (0x4A59E0 / 0x4A5EB0) is not replaced by this adapter.
void TechnoClass::DrawExtraInfo(const Point2D&, const Point2D& center, const RectangleStruct& bounds) const { std::abort(); }
