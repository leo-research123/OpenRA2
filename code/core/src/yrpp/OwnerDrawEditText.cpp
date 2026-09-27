// YR 0x00623880 edit/text-label rendering and 0x00777EA0 IME snapshot.
// Original OwnerDraw globals and BitFont state; no additional edit object.
// Native composition is already Unicode: OS ANSI/DBCS conversion is confined
// to the device adapter, while original-game builds retain the Win32 entry.
#include "yrpp/OwnerDraw.h"
#include "yrpp/BitFont.h"
#include "yrpp/Drawing.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <cwchar>
namespace {
bool fill(Surface* surface,RectangleStruct rect,WORD color){
    if(surface)return surface->FillRect(&rect,color);
    if(auto* frame=game::game_ui_frame()){
        game::RasterDrawingRequest r;r.target=frame->drawing.types.target;r.position={rect.X,rect.Y};
        r.clip=DSurface::WindowBounds;r.width=rect.Width;r.height=rect.Height;r.color=color;
        const auto status=game::submit_type_raster(frame->drawing.types,r);game::record_ui_drawing(status);
        return status==game::DrawingStatus::drawn || status==game::DrawingStatus::skipped;
    }
    return false;
}
}
int YRPP_FASTCALL OwnerDraw::DrawEditText(Surface* surface,RectangleStruct* rect,
    const wchar_t* text,int caret,BitFont* font,unsigned color,int* scroll,
    int focused,int password,int background,int animation) noexcept {
    if(!font || !rect || !scroll)return 0;
    if(!text)text=L"";
    const int length=int(std::wcslen(text));
    if(caret<0 || caret>length)caret=0;
    int ime_length=0,ime_active=0;
    if(focused){UpdateIMECompositionString();ime_length=IMECompositionStringLength;ime_active=IMEComposing;}
    // Reject inputs that would overrun the executable's 0x800-unit scratch area.
    // MessageList's 162-unit lines and 256-unit edit buffer cannot reach this.
    if(caret+ime_length>=0x800 || *scroll<0)return 0;
    wchar_t buffer[0x800];std::wcsncpy(buffer,text,0x800);
    if(ime_length){
        std::wcsncpy(buffer+caret,IMECompositionString,0x800-caret);
        std::wcsncpy(buffer+caret+ime_length,text+caret,0x800-caret-ime_length);
    }
    buffer[0x7FF]=0;
    const int total=int(std::wcslen(buffer));
    if(password)std::wmemset(buffer,L'*',total);
    const int effective=caret+((ime_active || ime_length)?IMECompositionCursorPos:0);
    if(effective<*scroll+5)*scroll=std::max(0,effective-5);
    while(*scroll<total && font->GetTextFit(buffer+*scroll,rect->Width-5,0,false)<effective-*scroll)++*scroll;
    // Invalid externally supplied offsets have no safe pointer interpretation.
    if(*scroll>total)return 0;
    int before=caret-*scroll;
    const int prefix=std::max(0,before);
    int visible_ime=ime_length;
    if(before<0){before+=ime_length;visible_ime=std::max(0,before);}
    const int suffix=std::max(0,total-ime_length-caret+std::min(0,before));
    auto* at=buffer+*scroll;
    if(background)fill(surface,{rect->X-1,rect->Y-1,font->GetTextWidth(at)+5,rect->Height+2},0);
    const auto print=[&](int count,unsigned ink){
        if(count<=0)return;
        const int advance=PrintTextFixedLength(ink,font,rect,at,count,0,0,surface,animation);
        rect->X+=advance;rect->Width=std::max(0,rect->Width-advance);at+=count;
    };
    print(prefix,color);
    int cursor=-1;
    if(visible_ime){
        const int position=effective-(prefix+*scroll);
        int after=-1;
        if(position>=0 && position<=visible_ime){after=visible_ime-position;visible_ime=position;}
        print(visible_ime,ImeCompositionTextColor);
        if(after>=0){cursor=rect->X;print(after,ImeCompositionTextColor);}
    }else cursor=rect->X;
    print(suffix,color);
    const bool draw_caret=focused && cursor>0 && !SuppressCaret;
    SuppressCaret=0;
    if(!draw_caret)return 0;
    // 0x00620050 is called twice for vertical lines with alpha=255. Its
    // 16-bit blend divides by 256, with zero destination contribution here.
    // Preserve that packed-channel rounding; modern fills need no surface lock.
    const auto channel=[&](int left,int right){const unsigned mask=(255u>>right)<<left;
        return (((unsigned(CaretColor)&mask)*255u)>>8)&mask;};
    const WORD ink=WORD(channel(Drawing::RedShiftLeft,Drawing::RedShiftRight)|
        channel(Drawing::GreenShiftLeft,Drawing::GreenShiftRight)|channel(Drawing::BlueShiftLeft,Drawing::BlueShiftRight));
    const int end=rect->Y+rect->Height-2;
    const RectangleStruct line{cursor,std::min(rect->Y,end),1,std::abs(end-rect->Y)+1};
    fill(surface,line,ink);
    return fill(surface,{line.X+1,line.Y,line.Width,line.Height},ink)?1:0;
}
