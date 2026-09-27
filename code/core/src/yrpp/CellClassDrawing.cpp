// YR 0x00480350 terrain/smudge submission. Actual Cell fields/type variants are the
// authority; the host resolves palettes and consumes borrowed draw resources.
#include "yrpp/CellClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/FileSystem.h"
#include "tactical_drawing.hpp"
#include <algorithm>
#include <bit>

namespace {
// Borrow the actual cell and the same resolved LightConvert palette as its TMP.
// This scope lasts only through SmudgeTypeClass::DrawIt's synchronous submit.
struct SmudgeCellDrawing {
    const CellClass& cell;
    const game::DrawingPaletteHandle* palette;
};
game::DrawingStatus smudge_cell(void* pointer, const CellStruct& coordinates,
        const game::DrawingPaletteHandle*& palette, int& intensity) noexcept {
    const auto& state = *static_cast<const SmudgeCellDrawing*>(pointer);
    if (coordinates != state.cell.MapCoords) return game::DrawingStatus::unavailable;
    palette = state.palette;
    intensity = std::bit_cast<std::int16_t>(state.cell.Intensity_Terrain);
    return game::DrawingStatus::drawn;
}
game::DrawingStatus smudge_height(void*, int height, int& pixels) noexcept {
    pixels = TacticalClass::AdjustForZ(height);
    return game::DrawingStatus::drawn;
}
}

void CellClass::DrawIt(const Point2D& unraised,const RectangleStruct& clip,bool skip) {
    auto* frame=game::tactical_drawing();if(!frame||!frame->drawing||skip)return;
    const auto& context=*frame->drawing;
    const auto draw=[&]() noexcept -> game::DrawingStatus {
    using game::DrawingStatus;
    if (clip.Width<=0 || clip.Height<=0) return DrawingStatus::skipped;
    if (!context.terrain_palette) return DrawingStatus::unavailable;
    try {
        IsometricTileTypeClass* base=nullptr; int variant=0;
        if (!GetTerrainTile(base,variant)) return DrawingStatus::unavailable;
        auto* owner=base;
        for (int i=0;i<variant && owner;++i) owner=owner->NextVariant;
        if (!owner) return DrawingStatus::unavailable;
        const auto* tmp=reinterpret_cast<const TMPStruct*>(owner->GetImage());
        if (!tmp || tmp->Columns<=0 || tmp->Rows<=0) return DrawingStatus::unavailable;
        const int sub=IsoTileTypeIndex==0xffff ? 0 : static_cast<unsigned char>(Height);
        const TMPImage* image=nullptr;
        if (!tmp->GetSubTile(sub%(tmp->Columns*tmp->Rows),image)) return DrawingStatus::skipped;
        const int level=static_cast<signed char>(Level);
        const std::int64_t x=unraised.X,y=std::int64_t(unraised.Y)-15*level+context.types.overlay_offset.Y;
        auto left=x,top=y,right=x+60,bottom=y+30;
        if (image->Flags&1) {
            const auto ex=x+image->ExtraX-image->X,ey=y+image->ExtraY-image->Y;
            left=std::min(left,ex); top=std::min(top,ey);
            right=std::max(right,ex+image->ExtraWidth); bottom=std::max(bottom,ey+image->ExtraHeight);
        }
        const bool tile_visible = right>clip.X && bottom>clip.Y &&
            left<std::int64_t(clip.X)+clip.Width && top<std::int64_t(clip.Y)+clip.Height;
        auto* smudge = SmudgeTypeClass::Array.GetItemOrDefault(SmudgeTypeIndex);
        // A multi-cell smudge can remain visible when its owning TMP is offscreen.
        // Its original data offset and SHP bounds are handled by DrawIt/backend.
        if (!tile_visible && !smudge) return DrawingStatus::skipped;
        if (y<INT32_MIN || y>INT32_MAX) return DrawingStatus::invalid_argument;
        int red=Color2_Red,green=Color2_Green,blue=Color2_Blue;
        const int shades=red==1000 && green==1000 && blue==1000 ? 53 :
            LightConvertClass::PrepareCellTint(red,green,blue,context.lighting_quality);
        auto types=context.types;
        const auto palette=context.terrain_palette(types.backend_context,FileSystem::ISOx_PAL,
            red,green,blue,shades,types.palette);
        if (palette!=DrawingStatus::drawn) return palette;
        auto result = DrawingStatus::skipped;
        if (tile_visible) {
            result = base->DrawTMP(types,sub,static_cast<int>(x),static_cast<int>(y),clip,level,
                std::bit_cast<std::int16_t>(Intensity_Terrain),true,variant,false,false,false,0);
            if (result!=DrawingStatus::drawn && result!=DrawingStatus::skipped) return result;
        }
        if (smudge) {
            // 0x004804A2: immediately after this cell's TMP, at its top vertex.
            // SHP takes clip-relative coordinates; TMP takes absolute coordinates.
            const auto sx=x+30-clip.X, sy=y-clip.Y;
            if (sx<INT32_MIN || sx>INT32_MAX || sy<INT32_MIN || sy>INT32_MAX)
                return DrawingStatus::invalid_argument;
            SmudgeCellDrawing state{*this,types.palette};
            types.world_context=&state;
            types.smudge_cell=smudge_cell;
            types.height_to_pixels=smudge_height;
            const auto drawn=smudge->DrawIt(types,{static_cast<int>(sx),static_cast<int>(sy)},
                clip,SmudgeData,level*Unsorted::LevelHeight,MapCoords);
            if (drawn!=DrawingStatus::skipped) return drawn;
        }
        return result;
    } catch (...) { return DrawingStatus::backend_failure; }
    };
    game::record_tactical_drawing(draw());
}
