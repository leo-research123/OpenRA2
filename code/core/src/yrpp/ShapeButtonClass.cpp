// EA REDALERT/SHAPEBTN.CPP, f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with EA Section 7;
// see code/third_party/ea/LICENSE.TXT. YR 69DCF0 / 69DE00 / 69DEB0.
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/Memory.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
ShapeButtonClass::ShapeButtonClass() noexcept
    : ToggleClass(0,0,0,0,0),IsToFlash(false),FlashDelay(0),FlashCounter(0),UseFlash(false),
      DrawPosition{},UseSidebarSurface(false),Drawer(nullptr),IsDrawn(false),IsAlpha(false),ShapeData(nullptr),IsShapeLoaded(false) {}
ShapeButtonClass::ShapeButtonClass(unsigned id,SHPStruct* shape,int x,int y,int width,int height,bool alpha) noexcept
    : ShapeButtonClass() { ID=static_cast<int>(id); X=x; Y=y; IsAlpha=alpha; SetShape(shape,width,height); }
void ShapeButtonClass::SetShape(SHPStruct* shape,int width,int height) {
    if (IsShapeLoaded && ShapeData) { YRMemory::Deallocate(ShapeData); IsShapeLoaded=false; }
    ShapeData=shape;
    Width=width ? width : shape ? shape->Width : 0;
    Height=height ? height : shape ? shape->Height : 0;
    MarkRedraw();
}
bool ShapeButtonClass::Draw(bool forced) {
    if (!ControlClass::Draw(forced) || !ShapeData) return false;
    const int frame=Disabled ? 2 : UseFlash ? (IsToFlash ? 3+int(IsOn) : int(IsOn)) : int(IsPressed);
    // Positions have already been published in whole-canvas coordinates by
    // the original sidebar/tab. Device backends do not know this is a button.
    game::draw_ui_shape(ShapeData,{X,Y},std::min(frame,int(ShapeData->Frames)-1));
    IsDrawn=true; return true;
}
