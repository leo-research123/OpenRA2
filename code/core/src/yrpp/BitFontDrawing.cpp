// BitFont 434780 MSB-first glyph raster and advance, through generic fills.
// Font choice, measurement and glyph interpretation remain in the original
// core class. No Godot font, text or UI node is involved.
#include "yrpp/BitFont.h"
#include "api/type_drawing.hpp"
#include <algorithm>
#include <cstdint>
#include <climits>

game::DrawingStatus BitFont::SubmitGlyph(const game::TypeDrawingContext& drawing,wchar_t character,
    int x,int y,const RectangleStruct& clip,WORD color,int& next_x) noexcept {
    using game::DrawingStatus;
    next_x=x;
    if (character==L'\t') {
        const auto advance=Unknown_28 ? std::int64_t(Unknown_28)+x-
            (std::int64_t(Unknown_28)+x-field_20)%Unknown_28 : x;
        if (advance<INT_MIN || advance>INT_MAX) return DrawingStatus::invalid_argument;
        next_x=int(advance); return DrawingStatus::skipped;
    }
    const auto* glyph=GetCharacterBitmap(character);
    if (!glyph) { glyph=static_cast<const unsigned char*>(Pointer_8); color^=0x5555; }
    if (!glyph || !InternalPTR) return DrawingStatus::unavailable;
    const int width=glyph[0],height=InternalPTR->FontHeight;
    const auto next=std::int64_t(x)+width+State_2C;
    if (next<INT_MIN || next>INT_MAX || std::int64_t(y)+height>INT_MAX ||
        width>InternalPTR->Stride*8) return DrawingStatus::invalid_argument;
    next_x=static_cast<int>(next);
    const auto left=std::max(std::int64_t(x),std::int64_t(clip.X));
    const auto top=std::max(std::int64_t(y),std::int64_t(clip.Y));
    const auto right=std::min(std::int64_t(x)+width,std::int64_t(clip.X)+clip.Width);
    const auto bottom=std::min(std::int64_t(y)+height,std::int64_t(clip.Y)+clip.Height);
    bool drawn=false;
    for (auto row=top;row<bottom;++row) {
        const auto* bits=glyph+1+std::size_t(row-y)*field_18;
        for (auto column=left;column<right;) {
            const auto bit=[&](std::int64_t n) {return (bits[(n-x)/8]&(0x80u>>((n-x)%8)))!=0;};
            if (!bit(column)) { ++column; continue; }
            const auto start=column++;
            while (column<right && bit(column)) ++column;
            game::RasterDrawingRequest request;
            request.target=drawing.target; request.position={int(start),int(row)}; request.clip=clip;
            request.width=int(column-start); request.height=1; request.color=color;
            const auto status=game::submit_type_raster(drawing,request);
            if (status!=DrawingStatus::drawn && status!=DrawingStatus::skipped) return status;
            drawn|=status==DrawingStatus::drawn;
        }
    }
    return drawn ? DrawingStatus::drawn : DrawingStatus::skipped;
}
