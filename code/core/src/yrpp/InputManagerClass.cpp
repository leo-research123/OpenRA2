// Original normalized virtual-key state (54F5C0 -> 53EC90). OS left/right
// swapping belongs to the device adapter before it submits normalized keys.
#include "yrpp/InputManagerClass.h"
#include "yrpp/GeneralStructures.h"
#include <algorithm>
InputManagerClass::InputManagerClass() noexcept
    : field_0{},field_4{},field_8{},field_C{},field_10{},Keycodes_b{},Keycodes_w{},field_314{},field_318{} {}
bool InputManagerClass::IsKeyPressed(int key) const {
    return key>=0 && key<256 && Keycodes_b[key]!=byte{};
}
void InputManagerClass::SetKeyState(int key,bool pressed) noexcept {
    if (key>=0 && key<256) Keycodes_b[key]=pressed ? byte{1} : byte{0};
}
void InputManagerClass::SetClickPosition(const Point2D& point) noexcept { field_0=point.X; field_4=point.Y; }
void InputManagerClass::Reset() noexcept {
    std::fill_n(Keycodes_b,256,byte{}); std::fill_n(Keycodes_w,256,0);
    field_0=field_4=field_8=field_C=field_10=field_314=field_318=0;
}
