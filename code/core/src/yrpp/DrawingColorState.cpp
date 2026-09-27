// Shared resource color state: TMP radar colors also need RGB packing.
#include "yrpp/Drawing.h"
#include "yrpp/CCToolTip.h"
#ifndef RA2_IMAGE_GAME
namespace {
RGBMode color_mode = static_cast<RGBMode>(2);
int red_left = 11, red_right = 3, green_left = 5, green_right = 2;
int blue_left = 0, blue_right = 3;
short half_mask = 0x7bef, quarter_mask = 0x39e7, eighth_mask = 0x18e3;
RGBClass tooltip_color(255,255,255), white(255,255,255);
}
RGBClass& RGBClass::White = white;
RGBMode& Drawing::ColorMode = color_mode;
// Both original names alias one color. Keep storage with color services so
// palette-only targets do not pull in the UI and its input/world dependencies.
RGBClass& Drawing::TooltipColor = tooltip_color;
RGBClass& CCToolTip::ToolTipTextColor = tooltip_color;
int& Drawing::RedShiftLeft = red_left;
int& Drawing::RedShiftRight = red_right;
int& Drawing::GreenShiftLeft = green_left;
int& Drawing::GreenShiftRight = green_right;
int& Drawing::BlueShiftLeft = blue_left;
int& Drawing::BlueShiftRight = blue_right;
short& Drawing::HalfbrightMask = half_mask;
short& Drawing::QuarterbrightMask = quarter_mask;
short& Drawing::EighthbrightMask = eighth_mask;
int& RGBClass::RedShiftLeft = red_left;
int& RGBClass::RedShiftRight = red_right;
int& RGBClass::GreenShiftLeft = green_left;
int& RGBClass::GreenShiftRight = green_right;
int& RGBClass::BlueShiftLeft = blue_left;
int& RGBClass::BlueShiftRight = blue_right;
const int (*Drawing::ZGradientTable)[6] = nullptr;
#endif
void Drawing::SetTooltipColorForSide(int side) noexcept {
    // Fixed colors initialized at 0x0072A940 / 0x0072A960 / 0x0072A980.
    TooltipColor=side==0 ? RGBClass{164,210,255} : RGBClass{255,255,0};
}
