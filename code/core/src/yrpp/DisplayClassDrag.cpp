// YR 0x4AC310 / 0x4AC380: tentative left press and rubber-band polling.
#include "yrpp/DisplayClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/YRMath.h"
#include "yrpp/Unsorted.h"
#include "x87_integer.hpp"
#include <bit>

#if !defined(RA2_YRPP_GAME)
void DisplayClass::ClearDragBand() {
    if (TacticalClass::Instance) TacticalClass::Instance->EndRubberBand();
    LeftPressAndDraggingRectangle = false;
    unknown_bool_11D0 = false;
    Unsorted::DragSelectAborted = true;
    SetCursor(MouseCursorType::Default, false);
}
void DisplayClass::LeftMouseButtonClick(Point2D* point) {
    if (!RepairMode && !PowerToggleMode && !SellMode && CurrentSWTypeIndex == -1 && !CurrentBuildingType) {
        unknown_bool_11D0 = true;
        LeftDownPosition = unknown_11DC = *point;
    }
}
void DisplayClass::LeftMouseButtonDown(const Point2D& point) {
    if (!LeftPressAndDraggingRectangle || PlanningMode || PlaceBeaconMode) {
        if (unknown_bool_11D0) {
            const auto dx = std::bit_cast<int>(unsigned(point.X) - unsigned(LeftDownPosition.X));
            const auto dy = std::bit_cast<int>(unsigned(point.Y) - unsigned(LeftDownPosition.Y));
            if (game::x87_integer(Math::sqrt(double(dx) * dx + double(dy) * dy)) > 4) {
                LeftPressAndDraggingRectangle = true;
                unknown_bool_11D0 = false;
                if (!PlanningMode && !PlaceBeaconMode) {
                    TacticalClass::Instance->Redrawing = true;
                    TacticalClass::Instance->StartRubberBand(LeftDownPosition);
                    MouseClass::Instance.MouseClass::UpdateCursor(MouseCursorType::Default, false);
                }
            }
        }
    } else {
        auto clamped = point;
        if (clamped.X < 0) clamped.X = 0;
        if (clamped.X >= DSurface::ViewBounds.Width) clamped.X = DSurface::ViewBounds.Width - 1;
        if (clamped.Y < 0) clamped.Y = 0;
        if (clamped.Y >= DSurface::ViewBounds.Height) clamped.Y = DSurface::ViewBounds.Height - 1;
        if (clamped != unknown_11DC) {
            TacticalClass::Instance->Redrawing = true;
            TacticalClass::Instance->ModifyRubberBand(clamped);
        }
    }
}
#endif
