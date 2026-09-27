#include "yrpp/BitText.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/Drawing.h"
#include "game_ui_runtime.hpp"
#include "yrpp/TextLabelClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include <new>
#include <cstddef>
#define BIND(type,name,addr) type& name=*reinterpret_cast<type*>(addr)
BIND(BitFont*,BitFont::Instance,0x89C4D0);
BIND(DSurface*,DSurface::Alternate,0x887310);
BIND(bool,Game::IsFocused,0xA8ED80);
BIND(RGBClass,RGBClass::White,0xA80220);
BIND(int,RGBClass::RedShiftLeft,0x8A0DD0);BIND(int,RGBClass::RedShiftRight,0x8A0DD4);
BIND(int,RGBClass::BlueShiftLeft,0x8A0DD8);BIND(int,RGBClass::BlueShiftRight,0x8A0DDC);
BIND(int,RGBClass::GreenShiftLeft,0x8A0DE0);BIND(int,RGBClass::GreenShiftRight,0x8A0DE4);
ColorStruct ColorScheme::HSVToRGB(const ColorStruct& hsv)noexcept{
    ColorStruct result;reinterpret_cast<ColorStruct*(__thiscall*)(const ColorStruct*,ColorStruct*)>(0x517440)(&hsv,&result);return result;
}
BIND(DSurface*,DSurface::Temp,0x887314);
BIND(RectangleStruct,DSurface::WindowBounds,0x886FB0);
BIND(DynamicVectorClass<ColorScheme*>,ColorScheme::Array,0xB054D0);
BIND(RulesClass*,RulesClass::Instance,0x8871E0);
BIND(GadgetClass*,GadgetClass::StuckOn,0x8B3E88);BIND(GadgetClass*,GadgetClass::LastList,0x8B3E8C);
BIND(GadgetClass*,GadgetClass::Focused,0x8B3E90);BIND(GadgetClass*,GadgetClass::Hovered,0x8B3E94);
BIND(int,OwnerDraw::IMECompositionStringLength,0xB73564);BIND(int,OwnerDraw::IMECompositionCursorPos,0xB73568);
BIND(int,OwnerDraw::IMEComposing,0xB7356C);BIND(int,OwnerDraw::SuppressCaret,0xAC48C8);
BIND(COLORREF,OwnerDraw::ImeCompositionTextColor,0xAC4618);BIND(COLORREF,OwnerDraw::CaretColor,0xAC184C);
wchar_t (&OwnerDraw::IMECompositionString)[0x101]=*reinterpret_cast<wchar_t(*)[0x101]>(0xB73318);
BIND(int,Drawing::RedShiftLeft,0x8A0DD0);BIND(int,Drawing::RedShiftRight,0x8A0DD4);
BIND(int,Drawing::BlueShiftLeft,0x8A0DD8);BIND(int,Drawing::BlueShiftRight,0x8A0DDC);
BIND(int,Drawing::GreenShiftLeft,0x8A0DE0);BIND(int,Drawing::GreenShiftRight,0x8A0DE4);
void YRPP_FASTCALL OwnerDraw::UpdateIMECompositionString() noexcept {reinterpret_cast<void(__cdecl*)()>(0x777EA0)();}
DWORD SystemTimer::GetMilliseconds()noexcept{return reinterpret_cast<DWORD(__stdcall*)()>(0x010D0000)();}
void YRPP_FASTCALL VocClass::PlayGlobal(int i,int p,float v,AudioController* c){reinterpret_cast<void(__fastcall*)(int,int,float,AudioController*)>(0x750920)(i,p,v,c);}
int BitFont::GetTextFit(const wchar_t* t,int w,int n,bool b)noexcept{return reinterpret_cast<int(__thiscall*)(BitFont*,const wchar_t*,int,int,bool)>(0x433F50)(this,t,w,n,b);}
bool BitFont::GetTextDimension(const wchar_t* t,int* w,int* h,int m){return reinterpret_cast<bool(__thiscall*)(BitFont*,const wchar_t*,int*,int*,int)>(0x433CF0)(this,t,w,h,m);}
int BitFont::GetTextWidth(const wchar_t* t,int m)noexcept{return reinterpret_cast<int(__thiscall*)(BitFont*,const wchar_t*,int)>(0x433ED0)(this,t,m);}
int BitFont::Blit(wchar_t c,int x,int y,int color){return reinterpret_cast<int(__thiscall*)(BitFont*,wchar_t,int,int,int)>(0x434120)(this,c,x,y,color);}
void BitFont::Lock(Surface* s){reinterpret_cast<void(__thiscall*)(BitFont*,Surface*)>(0x4348F0)(this,s);}
void BitFont::UnLock(Surface* s){reinterpret_cast<void(__thiscall*)(BitFont*,Surface*)>(0x434990)(this,s);}
void BitFont::SetField20(int x){reinterpret_cast<void(__thiscall*)(BitFont*,int)>(0x434110)(this,x);}
void BitFont::SetClipMode(bool b){reinterpret_cast<void(__thiscall*)(BitFont*,bool)>(0x433C90)(this,b);}
void BitFont::SetRectangle(LTRBStruct* r){reinterpret_cast<void(__thiscall*)(BitFont*,LTRBStruct*)>(0x433CA0)(this,r);}
void BitFont::SetColor(WORD c){reinterpret_cast<void(__thiscall*)(BitFont*,WORD)>(0x433C70)(this,c);}
game::DrawingStatus BitFont::SubmitGlyph(const game::TypeDrawingContext&,wchar_t,int,int,const RectangleStruct&,WORD,int&)noexcept{__debugbreak();return game::DrawingStatus::unavailable;}
namespace game {const GameUiInput* game_ui_input()noexcept{return nullptr;}
DrawingStatus submit_type_raster(const TypeDrawingContext&,const RasterDrawingRequest&)noexcept{__debugbreak();return DrawingStatus::unavailable;}
GameUiFrame* game_ui_frame()noexcept{return nullptr;}void record_ui_drawing(DrawingStatus)noexcept{__debugbreak();}}
extern "C" {
__declspec(dllexport) int __fastcall FontString(BitFont* f,void*,const wchar_t* t,int x,int y,int len,int anim){return f->DrawString(t,x,y,len,anim);}
__declspec(dllexport) int __stdcall TextPrint(BitFont* f,Surface* s,const wchar_t* t,int x,int y,int len,int anim){return BitText::Print(f,s,t,x,y,len,anim);}
__declspec(dllexport) int __fastcall FixedPrint(unsigned color,BitFont* f,RectangleStruct* r,const wchar_t* t,int len,int h,int v,Surface* s,int anim){return OwnerDraw::PrintTextFixedLength(color,f,r,t,len,h,v,s,anim);}
__declspec(dllexport) void __fastcall LabelConstruct(TextLabelClass* l,void*,wchar_t* t,int x,int y,int c,TextPrintType s){new(l)TextLabelClass(t,x,y,c,s);}
__declspec(dllexport) bool __fastcall LabelDraw(TextLabelClass* l,void*,bool forced){return l->TextLabelClass::Draw(forced);}
__declspec(dllexport) int __fastcall EditDraw(Surface* surface,RectangleStruct* rect,const wchar_t* text,int caret,BitFont* font,unsigned color,int* scroll,int focused,int password,int background,int animation){return OwnerDraw::DrawEditText(surface,rect,text,caret,font,color,scroll,focused,password,background,animation);}
int _fltused=0;
int __cdecl _purecall(){__debugbreak();return 0;}
std::size_t __cdecl wcslen(const wchar_t* p){return reinterpret_cast<std::size_t(__cdecl*)(const wchar_t*)>(0x7CA405)(p);}
wchar_t* __cdecl wcsncpy(wchar_t* d,const wchar_t* p,std::size_t n){return reinterpret_cast<wchar_t*(__cdecl*)(wchar_t*,const wchar_t*,std::size_t)>(0x7CA422)(d,p,n);}
wchar_t* __cdecl DrawingWmemset(wchar_t* d,wchar_t c,std::size_t n){for(std::size_t i=0;i<n;++i)d[i]=c;return d;}
auto DrawingWmemsetImport=&DrawingWmemset;
void* __cdecl memset(void* out,int v,std::size_t n){auto* p=static_cast<volatile unsigned char*>(out);for(std::size_t i=0;i<n;++i)p[i]=v;return out;}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void* __cdecl memcpy(void* out,const void* in,std::size_t n){auto* d=(volatile char*)out;auto* s=(const char*)in;for(std::size_t i=0;i<n;++i)d[i]=s[i];return out;}
}

void __cdecl operator delete(void*)noexcept{__debugbreak();}
void __cdecl operator delete(void* p,std::size_t)noexcept{::operator delete(p);}
#pragma comment(linker,"/alternatename:__imp__wmemset=_DrawingWmemsetImport")
