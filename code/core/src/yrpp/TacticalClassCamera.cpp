// Existing TacticalClass responsibilities, calibrated to gamemd 7b8a0685:
// 6D8640 (limits/clamp), 6D6000 and 6D8B30 (center -> actual view fields).
#include "yrpp/TacticalClass.h"

void TacticalClass::CameraCenterBounds(int map_width,const RectangleStruct& visible,
    const RectangleStruct& viewport,Point2D& minimum,Point2D& maximum) noexcept {
    minimum={viewport.Width/2+30*(2*visible.X-map_width),
        viewport.Height/2+15*(map_width+2*visible.Y-5)};
    maximum={minimum.X+60*visible.Width-viewport.Width,
        minimum.Y+(60*visible.Height+270)/2-viewport.Height};
}
bool TacticalClass::ClampCameraCenter(Point2D& center,const Point2D& minimum,
    const Point2D& maximum) noexcept {
    const auto before=center;
    // Preserve original branch order even for an inverted interval. Host policy
    // for maps smaller than a modern viewport is applied before this helper.
    if (center.Y<minimum.Y) center.Y=minimum.Y;
    else if (center.Y>maximum.Y) center.Y=maximum.Y;
    if (center.X<minimum.X) center.X=minimum.X;
    else if (center.X>maximum.X) center.X=maximum.X;
    return center!=before;
}
void TacticalClass::SetViewCenter(const Point2D& center,const RectangleStruct& viewport) noexcept {
    TacticalCoord1=TacticalCoord2=center;
    TacticalPos={center.X-viewport.Width/2,center.Y-viewport.Height/2};
    const auto cell=ApplyMatrix_Pixel({TacticalPos.X-60,TacticalPos.Y-30});
    ContainingMapCoords={cell.X/256,cell.Y/256,viewport.Width/60+2,viewport.Height/15+4};
    Redrawing=true;
}

#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#if !defined(RA2_YRPP_GAME)
// OpenTS 44fac744 tactical.cpp Set_Tactical_Position; YR 0x006D6070.
void TacticalClass::SetTacticalPosition(CoordStruct* coord) {
    if (!coord) return;
    auto center=CoordsToScreen(*coord);
    Point2D minimum,maximum;
    CameraCenterBounds(MapClass::Instance.MapRect.Width,MapClass::Instance.VisibleRect,ViewBounds,minimum,maximum);
    auto limited=center;
    if (ClampCameraCenter(limited,minimum,maximum) && !Unsorted::ArmageddonMode) center=limited;
    SetViewCenter(center,ViewBounds);
}
#endif
bool TacticalClass::FocusView(const Point2D& requested) noexcept {
    if (ViewBounds.Width<=0 || ViewBounds.Height<=0) return false;
    Point2D minimum,maximum;
    CameraCenterBounds(MapClass::Instance.MapRect.Width,MapClass::Instance.VisibleRect,ViewBounds,minimum,maximum);
    // Standalone oversized-view extension shared with the public camera API.
    if (minimum.X>maximum.X) minimum.X=maximum.X=(minimum.X+maximum.X)/2;
    if (minimum.Y>maximum.Y) minimum.Y=maximum.Y=(minimum.Y+maximum.Y)/2;
    auto center=requested; ClampCameraCenter(center,minimum,maximum);
    SetViewCenter(center,ViewBounds);
    return true;
}
