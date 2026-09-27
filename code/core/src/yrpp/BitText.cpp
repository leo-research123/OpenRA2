// YR 0x00434CD0 line breaking/alignment and character limit, with the existing
// BitFont glyph renderer. Surface locks become typed submissions on the host.
#include "yrpp/BitText.h"
#include "game_ui_runtime.hpp"
#include <algorithm>

namespace { BitText text_instance;BitText* text_pointer=&text_instance; }
BitText*& BitText::Instance=text_pointer;

void BitText::DrawText(BitFont* font,Surface* surface,const wchar_t* text,
    int x,int y,int width,int height,char alignment,int limit,int fade) {
    if(!font || !font->InternalPTR || !text)return;
    auto* frame=game::game_ui_frame();if(!surface && !frame)return;
    if(surface)font->Lock(surface);
    struct Unlock{BitFont* font;Surface* surface;~Unlock(){if(surface)font->UnLock(surface);}} unlock{font,surface};
    font->SetField20(x);
    const RectangleStruct clip{font->Bounds.Left,font->Bounds.Top,
        font->Bounds.Right-font->Bounds.Left+1,font->Bounds.Bottom-font->Bounds.Top+1};
    const auto base_color=font->Color;
    int lines=0,count=0,line_width=0,characters=0,space_width=0;
    const wchar_t* first=text;const wchar_t* space=nullptr;
    wchar_t previous=0;
    const auto flush=[&](const wchar_t* end)->bool{
        int at=x;
        if(width>line_width){if(alignment&1)at+=(width-line_width)/2;else if(alignment&2)at+=width-line_width;}
        for(;first<end;++first){
            if(limit && ++count>=limit)return false;
            WORD color=base_color;
            if(limit && fade>0 && limit-count<fade){
                const int ratio=std::clamp((count-limit+fade+1)*(255/fade),0,255);
                const auto mix=[&](int channel){return channel+((255-channel)*ratio>>8);};
                const int r=mix(((color>>11)&31)<<3),g=mix(((color>>5)&63)<<2),b=mix((color&31)<<3);
                color=WORD((r>>3)<<11|(g>>2)<<5|(b>>3));
            }
            if(surface)at=font->Blit(*first,at,y,color);
            else {int next=at;const auto status=font->SubmitGlyph(frame->drawing.types,*first,at,y,clip,color,next);
                game::record_ui_drawing(status);at=next;
                if(status!=game::DrawingStatus::drawn && status!=game::DrawingStatus::skipped)return false;}
        }
        return true;
    };
    for(const wchar_t* p=text;*p;++p){
        const wchar_t character=*p;
        bool newline=false;
        if(character==L'\t'){
            if(font->Unknown_28)line_width=font->Unknown_28+line_width-(font->Unknown_28+line_width)%font->Unknown_28;
        }else if(character==L'\r' || character==L'\n'){
            if(previous!=L'\r'){if(!flush(p))return;newline=true;}
            first=p+1;
        }else{
            if(character==L' '){space=p;space_width=line_width;}
            auto* glyph=font->GetCharacterBitmap(character);
            if(!glyph)glyph=font->GetCharacterBitmap(L'?');
            if(glyph){
                ++characters;const int advance=*glyph+font->State_2C;line_width+=advance;
                if(width && line_width>width){
                    if(space){if(characters>1){line_width=space_width;p=space;}}
                    else if(characters>1){line_width-=advance;--p;}
                    if(!flush(p))return;
                    if(p==space){space=nullptr;++first;}
                    newline=true;
                }
            }
        }
        if(newline){line_width=characters=0;lines+=font->field_1C;
            if(height && lines>=height)return;y+=font->field_1C;}
        previous=character;
        if(!p[1]){flush(p+1);return;}
    }
}
