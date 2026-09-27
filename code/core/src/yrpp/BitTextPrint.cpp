// Original 0x00434B90: no BitText object state, seven stdcall arguments.
#include "yrpp/BitText.h"
#include "game_ui_runtime.hpp"
#include "bitfont_drawing.hpp"
int YRPP_STDCALL BitText::Print(BitFont* font,Surface* surface,const wchar_t* text,
    int x,int y,int length,int animationPosition) noexcept {
    if(!font)return x;
    if(surface)font->Lock(surface);
    struct Unlock { BitFont* font;Surface* surface;~Unlock(){if(surface)font->UnLock(surface);} } unlock{font,surface};
    font->SetField20(x);
    if(!surface)if(auto* frame=game::game_ui_frame())
        return game::submit_bitfont_string(*font,text,x,y,length,animationPosition,*frame);
    return font->DrawString(text,x,y,length,animationPosition);
}
