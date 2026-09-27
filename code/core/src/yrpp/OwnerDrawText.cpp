// OwnerDraw original text path, YR 0x006211D0. Surface-less submissions use the
// existing native UI frame, with the same font state and inclusive clip bounds.
#include "yrpp/OwnerDraw.h"
#include "yrpp/BitFont.h"
#include "yrpp/BitText.h"
#include "yrpp/Drawing.h"
#include "game_ui_runtime.hpp"

int YRPP_FASTCALL OwnerDraw::PrintTextFixedLength(unsigned color,BitFont* font,
    RectangleStruct* rect,const wchar_t* text,int length,int horizontal,int vertical,Surface* surface,int animation) noexcept {
    if(!Game::IsFocused)return 0;
    if(!font)font=BitFont::Instance;
    if(!surface && !game::game_ui_frame())surface=DSurface::Alternate;
    if(!font || !rect || !text || (!surface && !game::game_ui_frame()))return 0;
    int width=0,height=0;
    font->GetTextDimension(text,&width,&height,rect->Width);
    int x=rect->X,y=rect->Y;
    switch(horizontal){
        case 1:x+=(rect->Width-width+1)/2;break;
        case 2:x+=(width+1)/-2;break;
        case 3:x+=-1-width;break;
        case 4:x+=rect->Width-width-1;break;
    }
    switch(vertical){
        case 1:y+=(rect->Height-height+1)/2;break;
        case 2:y+=(height+1)/-2;break;
        case 3:y+=-1-height;break;
    }
    LTRBStruct bounds{rect->X,rect->Y,rect->X+rect->Width,rect->Y+rect->Height};
    font->SetClipMode(true);font->SetRectangle(&bounds);
    RGBClass rgb(color);font->SetColor(WORD(rgb.ToInt()));
    return BitText::Print(font,surface,text,x,y,length,animation)-x;
}
