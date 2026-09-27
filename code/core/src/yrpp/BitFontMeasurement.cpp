// YR BitFont 0x00433ED0 / 0x00433F50, using the existing original font data.
#include "yrpp/BitFont.h"
int BitFont::GetTextWidth(const wchar_t* text,int maxWidth) noexcept {
    int width=maxWidth;GetTextDimension(text,&width,nullptr,maxWidth);return width;
}
int BitFont::GetTextFit(const wchar_t* text,int width,int maximum,bool breakAtWord) noexcept {
    if(!InternalPTR || !text)return 0;
    int pixels=0,count=0;
    for(const auto* p=text;*p;++p){
        const auto character=*p;
        if(character==L'\t'){
            // Original fonts initialize a nonzero tab width.
            if(Unknown_28)pixels=Unknown_28+pixels-(Unknown_28+pixels)%Unknown_28;
        }else if(character!=L'\r' && character!=L'\n'){
            auto* glyph=GetCharacterBitmap(character);
            if(!glyph)glyph=static_cast<unsigned char*>(Pointer_8);
            if(glyph)pixels+=*glyph+State_2C;
        }
        ++count;
        if(pixels>width){
            --count;
            if(breakAtWord && character>0x20){
                int found=count;
                while(found && text[found-1]>0x20)--found;
                if(found)return found;
            }
            return count;
        }
        if((maximum && count==maximum) || (breakAtWord && (character==L'\r' || character==L'\n')))return count;
    }
    return count;
}
