// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 mouse.cpp.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md. Original YR cursor state and driver calls.
#include "yrpp/MouseClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/FileFormats/SHP.h"

namespace {
Point2D hotspot(const MouseCursor& cursor) {
    const auto* shape=MouseClass::CursorShape;
    Point2D point{};
    // The YR driver requires loaded art when a cursor change is submitted.
    if(cursor.HotX==MouseHotSpotX::Center)point.X=shape->Width/2;
    if(cursor.HotX==MouseHotSpotX::Right)point.X=shape->Width;
    if(cursor.HotY==MouseHotSpotY::Middle)point.Y=shape->Height/2;
    if(cursor.HotY==MouseHotSpotY::Bottom)point.Y=shape->Height;
    return point;
}
int first_frame(const MouseCursor& cursor,bool mini) {
    return mini && cursor.MiniFrame!=-1?cursor.MiniFrame:cursor.Frame;
}
}
bool MouseClass::SetCursor(MouseCursorType index,bool mini) {
    MouseCursorLastIndex=index;
    return UpdateCursor(index,mini);
}
bool MouseClass::UpdateCursor(MouseCursorType index,bool mini) {
    // A native host can have input before installing its cursor device/art.
    // Report unavailable and leave the cursor uninitialized so later setup
    // can perform the original first draw. Original valid-device behavior below
    // is unchanged; this is not an emulation of an invalid original pointer.
    if(!CursorShape || !WWMouseClass::Instance)return false;
    const auto& cursor=MouseCursor::GetCursor(index);
    if(cursor.MiniFrame==-1)mini=false;
    if(CursorInitialized && (!CursorShape || (index==MouseCursorIndex && mini==MouseCursorIsMini)))return false;
    CursorInitialized=true;
    CursorTimer.Start(cursor.Interval);
    MouseCursorCurrentFrame=0;
    WWMouseClass::Instance->Draw(hotspot(cursor),CursorShape,first_frame(cursor,mini));
    MouseCursorIndex=index;
    MouseCursorIsMini=mini;
    return true;
}
bool MouseClass::RestoreCursor() {
    return UpdateCursor(MouseCursorLastIndex,false);
}
void MouseClass::UpdateCursorMinimapState(bool mini) {
    if(MouseCursorIsMini==mini)return;
    MouseCursorIsMini=mini;
    if(!CursorShape || !WWMouseClass::Instance)return;
    const auto& cursor=MouseCursor::GetCursor(MouseCursorIndex);
    // Unlike UpdateCursor this preserves mini=true for shapes without a small
    // frame. It also retains the animation frame and leaves the timer alone.
    WWMouseClass::Instance->Draw(hotspot(cursor),CursorShape,first_frame(cursor,mini)+MouseCursorCurrentFrame);
}
MouseCursorType MouseClass::GetLastMouseCursor() {
    return MouseCursorLastIndex;
}
