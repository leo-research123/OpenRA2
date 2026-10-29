// Existing TacticalClass responsibilities, calibrated to gamemd 7b8a0685:
// 6D8640 (limits/clamp), 6D6000 and 6D8B30 (center -> actual view fields).
#include "yrpp/TacticalClass.h"

void TacticalClass::CameraCenterBounds(int map_width, const RectangleStruct& visible, const RectangleStruct& viewport,
                                       Point2D& minimum, Point2D& maximum) noexcept {
    minimum = {viewport.Width / 2 + 30 * (2 * visible.X - map_width),
               viewport.Height / 2 + 15 * (map_width + 2 * visible.Y - 5)};
    maximum = {minimum.X + 60 * visible.Width - viewport.Width,
               minimum.Y + (60 * visible.Height + 270) / 2 - viewport.Height};
}
bool TacticalClass::ClampCameraCenter(Point2D& center, const Point2D& minimum, const Point2D& maximum) noexcept {
    const auto before = center;
    // Preserve original branch order even for an inverted interval. Host policy
    // for maps smaller than a modern viewport is applied before this helper.
    if (center.Y < minimum.Y)
        center.Y = minimum.Y;
    else if (center.Y > maximum.Y)
        center.Y = maximum.Y;
    if (center.X < minimum.X)
        center.X = minimum.X;
    else if (center.X > maximum.X)
        center.X = maximum.X;
    return center != before;
}
void TacticalClass::SetViewCenter(const Point2D& center, const RectangleStruct& viewport) noexcept {
    TacticalCoord1 = TacticalCoord2 = center;
    TacticalPos = {center.X - viewport.Width / 2, center.Y - viewport.Height / 2};
    const auto cell = ApplyMatrix_Pixel({TacticalPos.X - 60, TacticalPos.Y - 30});
    ContainingMapCoords = {cell.X / 256, cell.Y / 256, viewport.Width / 60 + 2, viewport.Height / 15 + 4};
    Redrawing = true;
}

#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SessionClass.h"
#include "yrpp/RulesClass.h"
#include "x87_integer.hpp"
#if !defined(RA2_YRPP_GAME)
// OpenTS Setup_Trigger_Scroll / AI; YR 0x6D2420 / 0x6D2540.
void TacticalClass::FocusOn(CoordStruct* destination, int velocity) {
    static constexpr float speeds[]{0.0015f, 0.003f, 0.0075f, 0.03f, 0.06f};
    field_DC = 0;
    field_D8 = speeds[velocity];
    Point_D0 = CoordsToScreen(*destination);
    Point_C8 = TacticalCoord1;
}

void TacticalClass::Update() {
    if (SessionClass::Instance.Play) return;
    bool moved = false;
    const auto publish = [this](Point2D center) {
        Point2D minimum, maximum;
        CameraCenterBounds(MapClass::Instance.MapRect.Width, MapClass::Instance.VisibleRect, ViewBounds, minimum,
                           maximum);
        // Preserve the existing standalone policy for oversized viewports.
        if (minimum.X > maximum.X) minimum.X = maximum.X = (minimum.X + maximum.X) / 2;
        if (minimum.Y > maximum.Y) minimum.Y = maximum.Y = (minimum.Y + maximum.Y) / 2;
        auto limited = center;
        if (ClampCameraCenter(limited, minimum, maximum) && !Unsorted::ArmageddonMode) center = limited;
        SetViewCenter(center, ViewBounds);
    };
    if (LastAIFrame != Unsorted::CurrentFrame) {
        if (Unsorted::ScenarioStarted && Point_D0 != Point2D{-1, -1} && field_D8 != 0) {
            const double progress = double(field_DC) + double(field_D8);
            field_DC = game::x87_double_to_float(progress);
            if (progress > 1.0) field_DC = 1.0f;
            const double remaining = 1.0 - double(field_DC);
            const Point2D center{game::x87_integer(double(Point_D0.X) * field_DC + double(Point_C8.X) * remaining),
                                 game::x87_integer(double(Point_D0.Y) * field_DC + double(Point_C8.Y) * remaining)};
            if (field_DC >= 1.0f) {
                Point_D0 = Point_C8 = {0, 0};
                field_D8 = field_DC = 0;
            }
            publish(center);
            moved = true;
        }
        if (!StartTime.GetTimeLeft()) {
            StartTime.Start(RulesClass::Instance->WaypointAnimationSpeed);
            ++MouseFrameIndex;
        }
    }
    LastAIFrame = Unsorted::CurrentFrame;
    if (TacticalCoord2 != TacticalCoord1 && !moved) publish(TacticalCoord2);
}

// OpenTS 44fac744 tactical.cpp Set_Tactical_Position; YR 0x006D6070.
void TacticalClass::SetTacticalPosition(CoordStruct* coord) {
    if (!coord) return;
    auto center = CoordsToScreen(*coord);
    Point2D minimum, maximum;
    CameraCenterBounds(MapClass::Instance.MapRect.Width, MapClass::Instance.VisibleRect, ViewBounds, minimum, maximum);
    auto limited = center;
    if (ClampCameraCenter(limited, minimum, maximum) && !Unsorted::ArmageddonMode) center = limited;
    SetViewCenter(center, ViewBounds);
}
#endif
bool TacticalClass::FocusView(const Point2D& requested) noexcept {
    if (ViewBounds.Width <= 0 || ViewBounds.Height <= 0) return false;
    Point2D minimum, maximum;
    CameraCenterBounds(MapClass::Instance.MapRect.Width, MapClass::Instance.VisibleRect, ViewBounds, minimum, maximum);
    // Standalone oversized-view extension shared with the public camera API.
    if (minimum.X > maximum.X) minimum.X = maximum.X = (minimum.X + maximum.X) / 2;
    if (minimum.Y > maximum.Y) minimum.Y = maximum.Y = (minimum.Y + maximum.Y) / 2;
    auto center = requested;
    ClampCameraCenter(center, minimum, maximum);
    SetViewCenter(center, ViewBounds);
    return true;
}

#if !defined(RA2_YRPP_GAME)
namespace {
constexpr Point2D scroll_deltas[]{{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
}
int TacticalClass::GetScrollDirection(int direction) {
    const auto delta = scroll_deltas[direction];
    auto next = TacticalCoord1 + delta;
    Point2D minimum, maximum;
    CameraCenterBounds(MapClass::Instance.MapRect.Width, MapClass::Instance.VisibleRect, ViewBounds, minimum, maximum);
    // Same native oversized-view policy as FocusView; original-size views use
    // the unchanged 0x6D8640 branch order.
    if (minimum.X > maximum.X) minimum.X = maximum.X = (minimum.X + maximum.X) / 2;
    if (minimum.Y > maximum.Y) minimum.Y = maximum.Y = (minimum.Y + maximum.Y) / 2;
    auto limited = next;
    if (ClampCameraCenter(limited, minimum, maximum) && !Unsorted::ArmageddonMode) next = limited;
    static constexpr int directions[]{7, 0, 1, 6, -1, 2, 5, 4, 3};
    return directions[4 + (TacticalCoord1.Y - next.Y) * 3 + TacticalCoord1.X - next.X];
}
void TacticalClass::ScrollBy(int direction, int distance) {
    TacticalCoord2.X += scroll_deltas[direction].X * distance;
    TacticalCoord2.Y += scroll_deltas[direction].Y * distance;
}
#endif
