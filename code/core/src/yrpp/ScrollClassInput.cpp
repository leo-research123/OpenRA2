// EA REDALERT/SCROLL.CPP f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc.; GPL-3.0-or-later with EA Section 7,
// see code/third_party/ea/LICENSE.TXT. YR 0x692B60 / 0x693440 calibration.
#include "yrpp/MouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Surface.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/YRMath.h"
#include "yrpp/WWMouseClass.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <cmath>
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

#if !defined(RA2_YRPP_GAME)
void ScrollClass::RightMouseButtonClick(Point2D* point) {
    if (Unsorted::UserInputLocked) return;
    DisplayClass::RightMouseButtonClick(point);
    unknown_int_5550 = static_cast<DWORD>(point->X);
    unknown_int_5554 = static_cast<DWORD>(point->Y);
    unknown_byte_5548 = 0;
    if (WWMouseClass::Instance) WWMouseClass::Instance->ShowCursor();
}
void ScrollClass::RightMouseButtonUp(DWORD miniMap) {
    if (!unknown_byte_5548) {
        DisplayClass::RightMouseButtonUp(miniMap);
        return;
    }
    SetCursor(MouseCursorType::Default, MouseClass::Instance.MouseCursorIsMini);
    unknown_byte_5548 = 0;
}
#endif
void ScrollClass::ResetScrollInput() noexcept {
    unknown_int_5548 = 0;
    unknown_byte_554C = unknown_byte_5548 = unknown_byte_554A = 0;
    unknown_int_5550 = unknown_int_5554 = 0;
    ScrollTimer.Stop();
    ScrollDirection = 0;
}
bool ScrollClass::DragScroll(const Point2D& point, Point2D& warp, bool& should_warp) noexcept {
    should_warp = false;
    if (!Game::IsActive || (!Unsorted::TacticalActive && !Unsorted::ArmageddonMode) || !TacticalClass::Instance)
        return false;
    const int origin_x = static_cast<int>(unknown_int_5550), origin_y = static_cast<int>(unknown_int_5554);
    int dx = point.X - origin_x, dy = point.Y - origin_y;
    // The native logical canvas uses the original default Win32 drag metric.
    // Device scaling belongs to the host, not the original mouse state.
    if (!unknown_byte_5548 && (std::abs(dx) > 8 || std::abs(dy) > 8)) unknown_byte_5548 = 1;
    if (!unknown_byte_5548) return false;
    if (!unknown_byte_554C) {
        if (LeftPressAndDraggingRectangle) {
            // The original passes a sentinel point through the legacy DWORD
            // slot. Scroll's override consumes the drag before that argument.
            RightMouseButtonUp(0);
        } else
            unknown_byte_554C = 1;
        if (!unknown_byte_554C) return false;
    }
    const auto& window = DSurface::WindowBounds;
    if (!dx) dx = origin_x == 0 ? -1 : origin_x >= window.Width - 1;
    if (!dy) dy = origin_y == 0 ? -1 : origin_y >= window.Height;
    if (origin_x < 10 && dx < 0)
        dx = std::min(dx, -5) * 4;
    else if (origin_x > window.Width - 10 && dx > 0)
        dx = std::max(dx, 5) * 4;
    if (origin_y < 10 && dy < 0)
        dy = std::min(dy, -5) * 4;
    else if (origin_y > window.Height - 10 && dy > 0)
        dy = std::max(dy, 5) * 4;
    const auto& options = GameOptionsClass::Instance;
    if (options.ScrollMethod == 2) {
        dx = -dx;
        dy = -dy;
    }
    const double numerator = options.ScrollMethod ? 12.0 : 1.0;
#if defined(_M_IX86) || defined(__i386__) || defined(__x86_64__)
    const double multiplier = game::x87_truncated_quotient(numerator, options.ScrollRate + 1);
#else
    const double multiplier = numerator / (options.ScrollRate + 1);
#endif
    // 0x00693646 keeps the quotient on x87. The first conversion changes
    // x87 rounding before the second multiply; SSE's MXCSR stays unchanged.
    int x_distance = std::abs(game::x87_scaled_quotient_to_integer(dx, multiplier, 1));
    int y_distance = std::abs(game::x87_scaled_quotient_to_integer(dy, multiplier, 1));
    if (options.ScrollMethod && x_distance + y_distance > 0) {
        warp = {origin_x, origin_y};
        should_warp = true;
    }
    unsigned blocked = 0;
    for (unsigned direction = 0; direction < 8; direction += 2) {
        int one = 1;
        if (!DisplayClass::ScrollMap(direction, &one, false)) blocked |= 1u << (direction / 2);
    }
    if (dx) DisplayClass::ScrollMap(dx > 0 ? 2 : 6, &x_distance, true);
    if (dy) DisplayClass::ScrollMap(dy > 0 ? 4 : 0, &y_distance, true);
    static constexpr int cursors[]{61, 62, 64, 63, 66, 61, 65, 61, 68, 69, 61, 61, 67, 61, 61, 61};
    return SetCursor(static_cast<MouseCursorType>(cursors[blocked]), true);
}
bool ScrollClass::ScrollAtEdge(const Point2D& local) noexcept {
    if (!TacticalClass::Instance || unknown_byte_554A == 1) return false;
    const Point2D point{local.X + DSurface::ViewBounds.X, local.Y + DSurface::ViewBounds.Y};
    const auto& window = DSurface::WindowBounds;
    const bool edge = point.X <= 0 || point.Y <= 0 || point.X >= window.Width - 1 || point.Y >= window.Height - 1;
    bool applied = false;
    if (unknown_int_5548 || edge) {
        if (edge) {
            const int x = point.X<window.Width * 0.16 ? 0 : point.X>(1.0 - 0.16) * window.Width ? window.Width - 1
                                                                                                : window.Width / 2;
            const int y = point.Y<window.Height * 0.21 ? 0 : point.Y>(1.0 - 0.21) * window.Height ? window.Height - 1
                                                                                                  : window.Height / 2;
            const auto angle = static_cast<unsigned short>(game::x87_integer(
                (Math::atan2(double(window.Height / 2 - y), double(x - window.Width / 2)) - Math::HalfPi) *
                -10430.060040584269));
            ScrollDirection = static_cast<unsigned char>(((angle >> 7) + 1) >> 1);
        }
        int rate = 8 - static_cast<int>(unknown_int_5548);
        const int minimum = GameOptionsClass::Instance.ScrollRate + 1;
        if (rate < minimum) {
            rate = minimum;
            unknown_int_5548 = 8 - minimum;
        }
        if (InputManagerClass::Instance && InputManagerClass::Instance->IsKeyPressed(2))
            rate = std::clamp(rate + 1, 4, 8);
        const auto direction = unsigned((((ScrollDirection << 8) >> 12) + 1) >> 1) & 7;
        int distance = 1;
        if (!ScrollMap(direction, &distance, false))
            UpdateCursor(static_cast<MouseCursorType>(direction + 9), false);
        else {
            UpdateCursor(static_cast<MouseCursorType>(direction + 1), false);
            static constexpr int speeds[]{448, 384, 320, 256, 192, 128, 64, 32, 16};
            distance = game::x87_integer(speeds[rate] * RulesClass::Instance->ScrollMultiplier);
            ScrollMap(direction, &distance, true);
            applied = true;
            if (!ScrollTimer.GetTimeLeft() && edge) {
                ScrollTimer.Start(1);
                ++unknown_int_5548;
            }
        }
    }
    if (!edge && !ScrollTimer.GetTimeLeft()) {
        if (unknown_int_5548) --unknown_int_5548;
        ScrollTimer.Start(1);
    }
    return applied;
}
void ScrollClass::PollMouse() {
    if (Unsorted::UserInputLocked) return;
    Point2D point;
    WWMouseClass::Instance->GetCoords(&point);
    point.X -= DSurface::ViewBounds.X;
    point.Y -= DSurface::ViewBounds.Y;
    if (Game::PlanningManager_UpdateCursor(point.X, point.Y)) return;
    if (unknown_byte_554A == 1) {
        if (InputManagerClass::Instance->IsKeyPressed(1))
            DisplayClass::Instance.DisplayClass::LeftMouseButtonDown(point);
        else if (InputManagerClass::Instance->IsKeyPressed(2)) {
            Point2D warp;
            bool should_warp = false;
            ScrollClass::Instance.DragScroll(point, warp, should_warp);
            if (should_warp)
                WWMouseClass::Instance->SetCoords({warp.X + DSurface::ViewBounds.X, warp.Y + DSurface::ViewBounds.Y});
        }
    } else {
        CellStruct cell;
        CoordStruct coords;
        ObjectClass* object = nullptr;
        BYTE fogged = 0, shrouded = 0;
        if (ProcessClickCoords(&point, &cell, &coords, &object, &fogged, &shrouded))
            DisplayClass::Instance.DisplayClass::ConvertAction(cell, shrouded != 0, object,
                                                               DecideAction(cell, object, 1), false);
        if (unknown_byte_5549 && !Unsorted::ArmageddonMode) ScrollAtEdge(point);
    }
}
void ScrollClass::Update(const int& key, const Point2D& point) {
    // Native startup can update presentation before a scenario or device is
    // installed. The original polling body requires those live dependencies.
    if (ScenarioClass::Instance && HouseClass::CurrentPlayer && WWMouseClass::Instance) PollMouse();
    if (HouseClass::CurrentPlayer) TabClass::Update(key, point);
}
