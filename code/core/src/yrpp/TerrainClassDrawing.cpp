// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 terrain.cpp::Draw_It.
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// YR 0x0071C1B0: no TS YDrawFudge; animated flags 0x2E00; shadow Z bias -3.
#include "yrpp/TerrainClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>
#include "sprite_drawing.hpp"
#include <bit>

void TerrainClass::DrawIt(Point2D* point, RectangleStruct* clip) const {
    auto* d = game::sprite_drawing();
    if (!d) return;
    if (!point || !clip || !Type) { d->status = game::DrawingStatus::invalid_argument; return; }
    if (!d->cell_at || !d->height || !d->shape_data) { d->status = game::DrawingStatus::unavailable; return; }
    CellStruct coords;
    auto* cell = d->cell_at(d->context, *GetMapCoords(&coords));
    auto* image = GetImage();
    if (!image) return;
    if (!cell) { d->status = game::DrawingStatus::unavailable; return; }
    int frame = Type->IsAnimated ? Animation.Value : IsCrumbling ? Animation.Value + 1 : Health < 2;
    const int z = -d->height(GetZ());
    if (!game::sprite_initialize_light(*d, *cell)) return;
    game::ShapeDrawingRequest r;
    r.image = image; r.frame = frame; r.position = *point; r.clip = *clip;
    r.flags = Type->IsAnimated || IsCrumbling ? 0x2E00 : 0x4E00;
    r.depth_adjustment = z - 12; r.gradient = 2;
    r.intensity = std::bit_cast<short>(Type->SpawnsTiberium ? cell->Intensity_Normal : cell->Intensity_Terrain);
    if (Type->SpawnsTiberium) r.position.Y -= 16;
    if (!game::sprite_submit(*d, Type->SpawnsTiberium ? game::SpritePalette::tiberium : game::SpritePalette::cell, nullptr, cell, r)) return;
    if (d->draw_shadows) {
        auto* shape = d->shape_data(image);
        if (!shape) { d->status = game::DrawingStatus::unavailable; return; }
        r.frame += shape->Frames / 2; r.flags |= 1;
        r.depth_adjustment = z - 3; r.gradient = 0; r.intensity = 1000;
        game::sprite_submit(*d, game::SpritePalette::cell, nullptr, cell, r);
    }
}

// OpenTS terrain.cpp Get_Render_Rect; YR 0x0071D160. Union keeps the original
// +1 extension only when the second rectangle extends the right/bottom edge.
RectangleStruct* TerrainClass::GetRenderDimensions(RectangleStruct* output) {
 auto* image=GetImage();if(!image||!TacticalClass::Instance){*output={};return output;}
 if(auto* ref=image->AsReference()){ref->Load();image=ref->Data;}
 if(!image||image->Frames<=0){*output={};return output;}
 auto a=image->GetFrameBounds(0),b=image->GetFrameBounds(image->Frames/2);
 if(a.Width<=0||a.Height<=0)a=b;
 else if(b.Width>0&&b.Height>0){
  if(a.X>b.X){a.Width+=a.X-b.X;a.X=b.X;}
  if(a.Y>b.Y){a.Height+=a.Y-b.Y;a.Y=b.Y;}
  if(a.X+a.Width<b.X+b.Width)a.Width=b.X+b.Width-a.X+1;
  if(a.Y+a.Height<b.Y+b.Height)a.Height=b.Y+b.Height-a.Y+1;
 }
 a.X+=unknown_rect_D0.Width-TacticalClass::Instance->TacticalPos.X-image->Width/2;
 a.Y+=unknown_rect_D0.Height-TacticalClass::Instance->TacticalPos.Y-image->Height/2;
 *output=a;return output;
}
