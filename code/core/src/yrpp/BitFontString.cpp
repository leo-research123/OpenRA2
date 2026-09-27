// YR 0x00434500 single-line character animation, calibrated with the fixed EXE.
// The existing BitFont owns glyph interpretation; the host changes only the
// final pixel destination, keeping the original returned advance and color.
#include "yrpp/BitFont.h"
#include "yrpp/Drawing.h"
#include "game_ui_runtime.hpp"
#include "bitfont_drawing.hpp"
#include <bit>

namespace {
int draw_string(BitFont& font,const wchar_t* text,int x,int y,int length,int animationPosition,game::GameUiFrame* frame) {
    if(!text)return x;
    const WORD base=font.Color;
    struct Restore { WORD& color; WORD value; ~Restore(){color=value;} } restore{font.Color,base};
    const RectangleStruct clip{font.Bounds.Left,font.Bounds.Top,font.Bounds.Right-font.Bounds.Left+1,font.Bounds.Bottom-font.Bounds.Top+1};
    int count=0;
    auto remaining=std::bit_cast<int>(unsigned(animationPosition)-1u);
    BYTE blend=BYTE(31u*(9u-unsigned(animationPosition)));
    for(auto* p=text;*p;++p){
        if(animationPosition){
            // Original measures the suffix AFTER the first omitted character.
            if(count>=animationPosition)return x+font.GetTextWidth(p+1);
            if(remaining<8){
                RGBClass color(base,true);color.Adjust(blend,RGBClass::White);font.Color=WORD(color.ToInt());
            }
            blend=BYTE(blend+31);++count;--remaining;
        }
        if(*p!=L'\r' && *p!=L'\n'){
            if(!frame)x=font.Blit(*p,x,y,-1);
            else {
                int next=x;
                const auto status=font.SubmitGlyph(frame->drawing.types,*p,x,y,clip,font.Color,next);
                game::record_ui_drawing(status);x=next;
                if(status!=game::DrawingStatus::drawn && status!=game::DrawingStatus::skipped)return x;
            }
        }
        if(length && !--length)break;
    }
    return x;
}

}
int BitFont::DrawString(const wchar_t* text,int x,int y,int length,int animationPosition) noexcept {
    return draw_string(*this,text,x,y,length,animationPosition,nullptr);
}
namespace game {
int submit_bitfont_string(BitFont& font,const wchar_t* text,int x,int y,int length,int animationPosition,GameUiFrame& frame){
    return draw_string(font,text,x,y,length,animationPosition,&frame);
}
}
