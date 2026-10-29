// Original mouse state; the native compositor presents a separate cursor plane.
// Saved-background surface operations belong to the original software device.
#include "yrpp/WWMouseClass.h"
#include <algorithm>

void WWMouseClass::Draw(const Point2D& hotspot, const SHPStruct* image, int frame) {
    if (image) {
        Image = const_cast<SHPStruct*>(image);
        ImageFrameIndex = frame;
        XY2 = hotspot;
    }
}
bool WWMouseClass::IsRefCountNegative() { return RefCount < 0; }
void WWMouseClass::HideCursor() { --RefCount; }
void WWMouseClass::ShowCursor() {
    if (RefCount < 0)
        ++RefCount;
    else
        RefCount = 0;
}
void WWMouseClass::ReleaseMouse() { field_10 = false; }
void WWMouseClass::CaptureMouse() { field_10 = true; }
byte WWMouseClass::IsCaptured() { return field_10; }
DWORD WWMouseClass::GetRefCount() { return DWORD(RefCount); }
int WWMouseClass::GetX() { return XY1.X; }
int WWMouseClass::GetY() { return XY1.Y; }
Point2D* WWMouseClass::GetCoords(Point2D* out) {
    *out = XY1;
    return out;
}
void WWMouseClass::SetCoords(Point2D point) { XY1 = point; }
void WWMouseClass::func_20(RectangleStruct) {}
void WWMouseClass::CallFunc10() { ShowCursor(); }
void WWMouseClass::func_3C(DSurface*, bool) {}
void WWMouseClass::func_40(DSurface*, bool) {}
void WWMouseClass::func_44(int* x, int* y) {
    *x = std::min(std::max(*x - Rect0.X, 0), Rect0.Width - 1);
    *y = std::min(std::max(*y - Rect0.Y, 0), Rect0.Height - 1);
}
